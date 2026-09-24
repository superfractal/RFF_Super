# Modified by GPT-6 on 2026-09-24
"""Inventory a Windows release against installed MSYS2 package records (stdlib only)."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

SYSTEM_DLLS = set(
    '''advapi32.dll bcrypt.dll cfgmgr32.dll comctl32.dll comdlg32.dll
crypt32.dll d3d11.dll dbghelp.dll dnsapi.dll dwmapi.dll dxgi.dll gdi32.dll gdiplus.dll
imm32.dll iphlpapi.dll kernel32.dll mpr.dll msvcrt.dll ncrypt.dll netapi32.dll
normaliz.dll ntdll.dll ole32.dll oleacc.dll oleaut32.dll opengl32.dll powrprof.dll
psapi.dll rpcrt4.dll secur32.dll setupapi.dll shell32.dll shlwapi.dll user32.dll
userenv.dll uiautomationcore.dll usp10.dll uxtheme.dll version.dll winhttp.dll
wininet.dll winmm.dll winspool.drv ws2_32.dll wtsapi32.dll'''.split()
)


def sha256(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def pe_imports(path):
    data = Path(path).read_bytes()

    def u16(at):
        return struct.unpack_from('<H', data, at)[0]

    def u32(at):
        return struct.unpack_from('<I', data, at)[0]

    if data[:2] != b'MZ':
        raise ValueError(f'Not a PE file: {path}')
    pe = u32(60)
    if data[pe : pe + 4] != b'PE\0\0' or u16(pe + 4) != 0x8664:
        raise ValueError(f'Not a Windows x64 PE: {path}')
    optional = pe + 24
    if u16(optional) != 0x20B:
        raise ValueError(f'Not PE32+: {path}')
    table = optional + u16(pe + 20)
    sections = [struct.unpack_from('<IIII', data, table + i * 40 + 8) for i in range(u16(pe + 6))]

    def offset(rva):
        for size, address, raw_size, raw in sections:
            if address <= rva < address + max(size, raw_size):
                at = raw + rva - address
                if at >= len(data):
                    break
                return at
        raise ValueError(f'Invalid PE RVA {rva:x}: {path}')

    def name(rva):
        at = offset(rva)
        end = data.index(b'\0', at)
        value = data[at:end].decode('ascii').lower()
        if not re.fullmatch(r'[a-z0-9_.+-]+\.(dll|drv)', value):
            raise ValueError(f'Invalid import name: {value}')
        return value

    result = set()
    count = u32(optional + 108)
    for entry, stride, name_at in [(1, 20, 12), (13, 32, 4)]:
        if count <= entry:
            continue
        rva, size = struct.unpack_from('<II', data, optional + 112 + entry * 8)
        if not rva:
            continue
        at = offset(rva)
        for pos in range(at, at + size, stride):
            if not any(data[pos : pos + stride]):
                break
            if entry == 13 and not (u32(pos) & 1):
                raise ValueError(f'Unsupported VA delay imports: {path}')
            result.add(name(u32(pos + name_at)))
        else:
            raise ValueError(f'Unterminated import table: {path}')
    return sorted(result)


def fields(path):
    result, key = {}, None
    for line in path.read_text(encoding='utf-8').splitlines():
        if line.startswith('%') and line.endswith('%'):
            key = line.strip('%')
            result[key] = []
        elif key and line:
            result[key].append(line)
    return result


def package_database(msys):
    packages, owners = {}, {}
    for folder in sorted((msys / 'var/lib/pacman/local').iterdir()):
        if not (folder / 'desc').is_file():
            continue
        desc = fields(folder / 'desc')
        pkg = desc['NAME'][0]
        if not pkg.startswith('mingw-w64-x86_64-'):
            continue
        items = fields(folder / 'files').get('FILES', [])
        packages[pkg] = {
            'name': pkg,
            'version': desc['VERSION'][0],
            'base': desc['BASE'][0],
            'homepage': desc.get('URL', [''])[0],
            'declared_licenses': desc.get('LICENSE', []),
            'files': items,
            'database': str(folder),
        }
        for item in items:
            if not item.endswith('/'):
                owners[item.lower()] = pkg
    return packages, owners


def expected_hash(pkg, relative):
    with gzip.open(Path(pkg['database']) / 'mtree', 'rt', encoding='utf-8') as stream:
        for line in stream:
            if line.lower().startswith('./' + relative.lower() + ' '):
                match = re.search(r'(?:sha256digest|sha256)=([0-9a-f]{64})', line)
                if match:
                    return match[1]
    raise ValueError(f'No installed package digest for {relative}')


def make_inventory(exe, msys, static_gmp, project):
    packages, owners = package_database(msys)
    pending, seen, artifacts, required, system = [exe], set(), [], {}, set()
    while pending:
        path = pending.pop()
        if path.name.lower() in seen:
            continue
        seen.add(path.name.lower())
        imports = pe_imports(path)
        item = {'file': path.name, 'path': str(path), 'sha256': sha256(path), 'imports': imports}
        if path != exe:
            relative = 'mingw64/bin/' + path.name
            pkg = packages[owners[relative.lower()]]
            expected = expected_hash(pkg, relative)
            if item['sha256'] != expected:
                raise ValueError(f'Package digest mismatch: {path}')
            required[pkg['name']] = pkg
            item['package'] = pkg['name']
        artifacts.append(item)
        for dll in imports:
            if dll == 'vulkan-1.dll':
                system.add(dll)
                continue
            if dll in SYSTEM_DLLS or dll.startswith(('api-ms-win-', 'ext-ms-win-')):
                system.add(dll)
                continue
            relative = 'mingw64/bin/' + dll
            if relative not in owners:
                raise ValueError(f'Unowned non-system DLL {dll} imported by {path.name}')
            dep = msys / relative
            if not dep.is_file():
                raise ValueError(f'Missing runtime dependency: {dep}')
            pending.append(dep)
    gmp = packages['mingw-w64-x86_64-gmp']
    if sha256(static_gmp) != expected_hash(gmp, 'mingw64/lib/libgmp.a'):
        raise ValueError('Static GMP does not match the installed package version')
    required[gmp['name']] = gmp
    for name in ['mingw-w64-x86_64-glm', 'mingw-w64-x86_64-headers', 'mingw-w64-x86_64-crt']:
        required[name] = packages[name]
    result_packages = []
    for pkg in sorted(required.values(), key=lambda p: p['name']):
        result = {k: v for k, v in pkg.items() if k not in ('files', 'database')}
        result['license_files'] = [
            str(msys / f) for f in pkg['files'] if '/share/licenses/' in f and not f.endswith('/')
        ]
        version = pkg['version'].split(':')[-1]
        result['source_archive'] = f"{pkg['base']}-{version}.src.tar.zst"
        result['source_url'] = 'https://repo.msys2.org/mingw/sources/' + result['source_archive']
        result_packages.append(result)
    revision = subprocess.check_output(
        ['git', '-C', str(project), 'rev-parse', 'HEAD'], text=True
    ).strip()
    return {
        'schema': 1,
        'status': 'INVENTORY_ONLY_NOT_A_RELEASE_APPROVAL',
        'revision': revision,
        'executable': str(exe),
        'static_gmp': {'path': str(static_gmp), 'sha256': sha256(static_gmp)},
        'artifacts': sorted(artifacts, key=lambda a: a['file'].lower()),
        'system_dependencies': sorted(system),
        'packages': result_packages,
        'external_tools': [
            'FFmpeg (user supplied)',
            'optional local AI server/models (user supplied)',
        ],
        'notes': [
            'Vulkan loader/ICD supplied by the installed GPU driver; never copied from bin.',
            'Normal and delay PE imports inspected; optional runtime loading also requires review.',
            'Source URLs are candidates until matched to the official source index and downloaded.',
            'Package license metadata is evidence, not a complete license determination.',
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--msys-root', type=Path, default=Path('C:/msys64'))
    parser.add_argument('--static-gmp', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = make_inventory(
        args.exe.resolve(),
        args.msys_root.resolve(),
        args.static_gmp.resolve(),
        Path(__file__).resolve().parents[1],
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(
        f"Inventoried {len(result['artifacts']) - 1} DLLs and {len(result['packages'])} packages: {args.output}"
    )


if __name__ == '__main__':
    main()
