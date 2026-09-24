# Modified by GPT-6 on 2026-09-24
"""Stage an allowlisted release candidate, with exact third-party sources and notices."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
from release_inventory import sha256, pe_imports, package_database, expected_hash


def copy_checked(source, target, expected=None):
    if expected and sha256(source) != expected:
        raise ValueError(f'Content changed since inventory: {source}')
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    if sha256(source) != sha256(target):
        raise ValueError(f'Copy verification failed: {target}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', required=True, type=Path)
    parser.add_argument('--sources', required=True, type=Path)
    parser.add_argument('--shaders', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--msys-root', type=Path, default=Path('C:/msys64'))
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    inventory = json.loads(args.inventory.read_text(encoding='utf-8'))
    sources = json.loads((args.sources / 'sources.json').read_text(encoding='utf-8'))
    reviewed = json.loads(
        (project / 'tools/release-license-review.json').read_text(encoding='utf-8')
    )
    expected_versions = reviewed['package_versions']
    if {p['name']: p['version'] for p in inventory['packages']} != expected_versions:
        raise ValueError('Dependency versions differ from the reviewed set; review before staging')
    records = {p['file']: p for p in sources['sources']}
    if args.output.exists():
        raise ValueError('Output already exists; choose a fresh candidate directory')
    args.output.mkdir(parents=True)
    stage = args.output.resolve()
    (stage / 'NOT-FOR-DISTRIBUTION.txt').write_text(
        'Release candidate only. The application source archive is not included.\n'
        'Before distribution, rebuild from a clean committed tag, archive that tag, and supply\n'
        'the corresponding application source and this third-party source/notice bundle together.\n',
        encoding='utf-8',
    )
    packages, _ = package_database(args.msys_root)
    for artifact in inventory['artifacts']:
        source = Path(artifact['path'])
        copy_checked(source, stage / 'bin' / artifact['file'], artifact['sha256'])
        independent = subprocess.check_output(
            [str(args.msys_root / 'mingw64/bin/objdump.exe'), '-p', str(source)], text=True
        )
        imported = sorted(set(n.lower() for n in re.findall(r'DLL Name:\s*(\S+)', independent)))
        if imported != pe_imports(source):
            raise ValueError(f'Independent import inspection disagrees: {source}')
    shader_names = [
        p.name + '.spv'
        for p in (project / 'shdsrc').iterdir()
        if p.suffix in ('.frag', '.vert', '.comp')
    ]
    shader_names.append('vk_2_map_iter_stripe_hdr.comp.spv')
    for name in shader_names:
        copy_checked(args.shaders / name, stage / 'shaders' / name)
    for name in (
        'LICENSE',
        'NOTICE',
        'README.md',
        'documentation/BUILDING.md',
        'documentation/DISTRIBUTION.md',
    ):
        copy_checked(project / name, stage / name)
    for file in (project / 'extern/licenses').iterdir():
        if file.is_file():
            copy_checked(file, stage / 'licenses/bundled' / file.name)
    copied_sources = set()
    for package in inventory['packages']:
        for name in package['license_files']:
            source = Path(name)
            relative = source.relative_to(args.msys_root).as_posix()
            digest = expected_hash(packages[package['name']], relative)
            copy_checked(source, stage / 'licenses' / package['name'] / source.name, digest)
        name = package['source_archive']
        if name in copied_sources:
            continue
        copied_sources.add(name)
        record = records[name]
        source = args.sources / name
        copy_checked(source, stage / 'sources/third-party' / name, record['sha256'])
        copy_checked(
            args.sources / (name + '.sig'),
            stage / 'sources/third-party' / (name + '.sig'),
            record['signature_sha256'],
        )
        copy_checked(
            args.sources / (name + '.verification.txt'),
            stage / 'sources/third-party' / (name + '.verification.txt'),
        )
        members = subprocess.check_output(['tar.exe', '-tf', str(source)], text=True).splitlines()
        for member in members:
            if not member.endswith(('.tar.gz', '.tar.xz', '.tar.bz2', '.tgz')):
                continue
            with tempfile.TemporaryFile(dir=stage) as nested:
                subprocess.run(['tar.exe', '-xOf', str(source), member], stdout=nested, check=True)
                nested.seek(0)
                with tarfile.open(fileobj=nested, mode='r|*') as upstream:
                    for item in upstream:
                        if not item.isfile() or item.size > 2 * 1024 * 1024:
                            continue
                        if not re.match(
                            r'^(?:LICEN[CS]E|COPYING|NOTICE|PATENTS).*$', Path(item.name).name, re.I
                        ):
                            continue
                        parts = Path(item.name).parts
                        if '..' in parts or Path(item.name).drive or Path(item.name).is_absolute():
                            raise ValueError('Unsafe source member')
                        dest = stage / 'licenses/source-notices' / package['base'] / Path(*parts)
                        dest.parent.mkdir(parents=True, exist_ok=True)
                        dest.write_bytes(upstream.extractfile(item).read())
    copy_checked(args.sources / 'sources.json', stage / 'sources/third-party/sources.json')
    copy_checked(project / 'tools/release-license-review.json', stage / 'licenses/review.json')
    (stage / 'DEPENDENCIES.json').write_text(
        json.dumps(inventory, indent=2) + '\n', encoding='utf-8'
    )
    manifest = {
        p.relative_to(stage).as_posix(): sha256(p) for p in sorted(stage.rglob('*')) if p.is_file()
    }
    (stage / 'SHA256SUMS.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'Candidate staged and independently checked: {stage}')
    print(
        f'{len(inventory["artifacts"])-1} DLLs; {len(shader_names)} shaders; {len(copied_sources)} source packages'
    )


if __name__ == '__main__':
    main()
