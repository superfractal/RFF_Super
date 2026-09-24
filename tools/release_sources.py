# Modified by GPT-6 on 2026-09-24
"""Fetch the exact MSYS2 source packages and validate their distribution signatures."""
import argparse
import base64
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import subprocess
import urllib.request
from release_inventory import sha256

BASE = 'https://repo.msys2.org/mingw/sources/'


def msys_path(path):
    path = path.resolve().as_posix()
    if len(path) > 2 and path[1] == ':':
        return '/' + path[0].lower() + path[2:]
    return path


def fetch(url, path):
    if path.is_file():
        return
    temporary = path.with_name(path.name + '.partial')
    with urllib.request.urlopen(url, timeout=90) as response, temporary.open('wb') as out:
        while block := response.read(1024 * 1024):
            out.write(block)
    temporary.replace(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--msys-root', type=Path, default=Path('C:/msys64'))
    args = parser.parse_args()
    inventory = json.loads(args.inventory.read_text(encoding='utf-8'))
    args.output.mkdir(parents=True, exist_ok=True)
    keyring = args.output / 'msys2-verification-keyring.gpg'
    armored = (args.msys_root / 'usr/share/pacman/keyrings/msys2.gpg').read_text(encoding='ascii')
    blocks = re.findall(
        r'-----BEGIN PGP PUBLIC KEY BLOCK-----\s*\n(.*?)-----END PGP PUBLIC KEY BLOCK-----',
        armored,
        re.S,
    )
    if not blocks:
        raise ValueError('MSYS2 package keyring contains no public key blocks')
    keyring.write_bytes(
        b''.join(
            base64.b64decode(
                ''.join(
                    line.strip()
                    for line in block.splitlines()
                    if line.strip() and not line.startswith('=') and ':' not in line
                ),
                validate=True,
            )
            for block in blocks
        )
    )
    index = args.output / 'official-index.html'
    fetch(BASE, index)
    available = set(re.findall(r'href="([^"]+)"', index.read_text(encoding='utf-8')))
    archives = sorted({p['source_archive'] for p in inventory['packages']})
    for name in archives:
        if name not in available or name + '.sig' not in available:
            raise ValueError(f'Exact source package absent from official index: {name}')

    def download(name):
        path = args.output / name
        fetch(BASE + name, path)
        fetch(BASE + name + '.sig', path.with_name(name + '.sig'))
        checked = subprocess.run(
            [
                str(args.msys_root / 'usr/bin/gpgv.exe'),
                '--keyring',
                msys_path(keyring),
                msys_path(path.with_name(name + '.sig')),
                msys_path(path),
            ],
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace',
        )
        if checked.returncode:
            raise ValueError(f'Signature verification failed: {name}\n{checked.stderr}')
        path.with_name(name + '.verification.txt').write_text(checked.stderr, encoding='utf-8')
        print('Verified ' + name, flush=True)
        return {
            'file': name,
            'url': BASE + name,
            'sha256': sha256(path),
            'signature_sha256': sha256(path.with_name(name + '.sig')),
        }

    with ThreadPoolExecutor(max_workers=3) as pool:
        records = list(pool.map(download, archives))
    (args.output / 'sources.json').write_text(
        json.dumps({'schema': 1, 'sources': records}, indent=2) + '\n', encoding='utf-8'
    )
    print(
        f'Verified {len(records)} source packages. Inspect embedded recipes and licenses before release.'
    )


if __name__ == '__main__':
    main()
