"""Verify reference hook bytes against the pinned, unmodified game executable."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]


def verify(game, evidence):
    data = game.read_bytes()
    if hashlib.sha256(data).hexdigest() != evidence['sha256'].lower():
        raise ValueError('Executable differs from the supported reference baseline')
    if len(data) != evidence['size'] or data[:2] != b'MZ':
        raise ValueError('Invalid reference executable size or DOS header')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Invalid PE signature')
    machine, count = struct.unpack_from('<HH', data, pe + 4)
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    if machine != 0x14c or struct.unpack_from('<H', data, optional)[0] != 0x10b:
        raise ValueError('Reference engine must be PE32 x86')
    base = struct.unpack_from('<I', data, optional + 28)[0]
    if base != int(evidence['image_base'], 16):
        raise ValueError('Reference image base differs')
    sections = []
    for index in range(count):
        offset = optional + optional_size + 40 * index
        name = data[offset:offset + 8].rstrip(b'\0').decode('ascii')
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', data, offset + 8)
        sections.append((name, rva, raw_size, raw_offset))
    verified = []
    for hook in evidence['hooks']:
        rva = int(hook['va'], 16) - base
        matched = [s for s in sections if s[1] <= rva < s[1] + s[2]]
        if len(matched) != 1:
            raise ValueError('Hook is not backed by one PE section: ' + hook['name'])
        name, section_rva, _, raw_offset = matched[0]
        offset = raw_offset + rva - section_rva
        expected = bytes.fromhex(hook['first_16_bytes'])
        if name != evidence['section'] or offset != int(hook['file_offset'], 16):
            raise ValueError('Hook section/offset differs: ' + hook['name'])
        if data[offset:offset + len(expected)] != expected or not expected.startswith(bytes.fromhex(hook['expected_prefix'])):
            raise ValueError('Hook bytes differ: ' + hook['name'])
        verified.append(hook['name'])
    return {'status': 'PASS', 'game_sha256': evidence['sha256'].lower(),
            'verified_hooks': verified,
            'scope': 'Static executable identity, PE offsets and recorded bytes; not native gameplay acceptance.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, default=ROOT / 'validation/engine-hooks.json')
    args = parser.parse_args()
    print(json.dumps(verify(args.game, json.loads(args.evidence.read_text())), indent=2))


if __name__ == '__main__':
    main()
