"""Replace local diagnostic path prefixes in an unsigned PE32 DLL."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]


def sanitize(data, prefix):
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    optional = pe + 24
    assert data[pe:pe + 4] == b'PE\0\0'
    assert struct.unpack_from('<H', data, optional)[0] == 0x10b
    assert not any(struct.unpack_from('<II', data, optional + 96 + 4 * 8)), 'Signed DLL cannot be sanitized.'
    table = optional + struct.unpack_from('<H', data, pe + 20)[0]
    sections = []
    for i in range(struct.unpack_from('<H', data, pe + 6)[0]):
        at = table + i * 40
        name = data[at:at + 8].rstrip(b'\0')
        size, offset = struct.unpack_from('<II', data, at + 16)
        flags = struct.unpack_from('<I', data, at + 36)[0]
        sections.append((name, offset, size, flags))
    before = prefix.encode('ascii')
    assert len(before) >= 4
    after = b'X:\\' + b'_' * (len(before) - 4) + b'\\'
    output = bytearray(data)
    edits = []
    offset = 0
    while (offset := data.find(before, offset)) >= 0:
        section = next(s for s in sections if s[1] <= offset < s[1] + s[2])
        end = data.find(b'\0', offset, offset + 4096)
        # Upstream assertion-site filenames are writable static arrays in .data.
        # Both forms are diagnostic C strings; never touch executable sections.
        assert section[0] in (b'.rdata', b'.data') and not section[3] & 0x20000000
        assert offset < end < section[1] + section[2]
        assert data[offset:end].lower().endswith((b'.cpp', b'.h', b'.c', b'.pdb', b'.hpp'))
        output[offset:offset + len(before)] = after
        edits.append({'offset': offset, 'length': len(before), 'replacement': after.decode()})
        offset += len(before)
    for _, start, length, flags in sections:
        if flags & 0x20000000:
            assert output[start:start + length] == data[start:start + length]
    assert len(output) == len(data)
    return bytes(output), edits


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=ROOT / 'build/Vinifera.dll')
    parser.add_argument('--out', type=Path, default=ROOT / 'build/public/Vinifera.dll')
    args = parser.parse_args()
    data = args.input.read_bytes()
    output, edits = sanitize(data, str(ROOT) + '\\')
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(output)
    receipt = {'input_sha256': hashlib.sha256(data).hexdigest(), 'output_sha256': hashlib.sha256(output).hexdigest(),
               'executable_sections_unchanged': True, 'diagnostic_prefix_replacements': edits}
    (args.out.parent / 'sanitation.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('Sanitized diagnostic prefixes:', len(edits))


if __name__ == '__main__':
    main()
