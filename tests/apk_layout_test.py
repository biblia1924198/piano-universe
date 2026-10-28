#!/usr/bin/env python3
"""Inspect actual signed APK local headers; signature verification is separate.

Usage: python3 tests/apk_layout_test.py signed.apk
Regression: --regression rejected-v051.apk corrected.apk working-v04.apk
"""
from pathlib import Path
import hashlib
import struct
import sys
import zipfile
import zlib


def check(path):
    with open(path, 'rb') as raw, zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None, 'ZIP CRC failed'
        entries = archive.infolist()
        assert len({item.filename for item in entries}) == len(entries), 'Duplicate ZIP entries'
        table = archive.getinfo('resources.arsc')
        assert table.compress_type == zipfile.ZIP_STORED, 'resources.arsc is compressed: Android 11+ rejects this APK'
        for item in entries:
            raw.seek(item.header_offset)
            header = raw.read(30)
            assert header[:4] == b'PK\x03\x04', f'Invalid local header: {item.filename}'
            assert struct.unpack_from('<H', header, 8)[0] == item.compress_type, 'ZIP method mismatch'
            name_len, extra_len = struct.unpack_from('<HH', header, 26)
            offset = item.header_offset + 30 + name_len + extra_len
            if item.compress_type == zipfile.ZIP_STORED:
                required = 16384 if item.filename.startswith('lib/') and item.filename.endswith('.so') else 4
                assert offset % required == 0, f'{item.filename} offset {offset} is not aligned to {required}'
            assert not item.flag_bits & 1, 'Encrypted entry'
        dex = archive.read('classes.dex')
        assert dex[:4] == b'dex\n' and len(dex) == struct.unpack_from('<I', dex, 32)[0], 'DEX header/length'
        assert hashlib.sha1(dex[32:]).digest() == dex[12:32], 'DEX digest'
        assert zlib.adler32(dex[12:]) & 0xffffffff == struct.unpack_from('<I', dex, 8)[0], 'DEX checksum'
        for name in ('lib/arm64-v8a/libpianoroom.so', 'lib/x86_64/libpianoroom.so'):
            elf = archive.read(name)
            assert elf[:6] == b'\x7fELF\x02\x01', 'Expected ELF64 little endian'
            phoff = struct.unpack_from('<Q', elf, 32)[0]
            phsize, count = struct.unpack_from('<HH', elf, 54)
            loads = 0
            for i in range(count):
                segment = phoff + i * phsize
                if struct.unpack_from('<I', elf, segment)[0] == 1:
                    loads += 1
                    offset, virtual = struct.unpack_from('<QQ', elf, segment + 8)
                    alignment = struct.unpack_from('<Q', elf, segment + 48)[0]
                    assert alignment >= 16384 and offset % 16384 == virtual % 16384, 'ELF LOAD 16 KiB alignment'
            assert loads > 0, 'ELF without LOAD segments'
    print(f'PASS: {Path(path).name}: resource table STORED/aligned; local ZIP headers, CRC, DEX, native ELF16KiB')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--regression':
        try:
            check(sys.argv[2])
        except AssertionError as error:
            assert 'resources.arsc is compressed' in str(error), str(error)
            print(f'PASS: prior rejected APK reproduces resource-table failure: {error}')
        else:
            raise AssertionError('Regression fixture unexpectedly accepted')
        for path in sys.argv[3:]:
            check(path)
    else:
        check(sys.argv[1])
