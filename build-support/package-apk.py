#!/usr/bin/env python3
"""Assemble an unsigned APK. Resource compression/alignment survive repackaging.

Sign the output afterward. Android 11+ rejects compressed or unaligned
resources.arsc even when the APK signature is valid.
"""
import argparse
from pathlib import Path
import struct
import zipfile


def write_entry(archive, name, data, method):
    info = zipfile.ZipInfo(name)
    info.compress_type = method
    info.external_attr = 0o100644 << 16
    if method == zipfile.ZIP_STORED:
        alignment = 16384 if name.startswith('lib/') and name.endswith('.so') else 4
        offset = archive.fp.tell() + 30 + len(name.encode('utf-8'))
        padding = (-offset) % alignment
        if padding:
            # A well-formed unknown ZIP extra field pads the local data offset.
            if padding < 4:
                padding += alignment
            info.extra = struct.pack('<HH', 0xFFFF, padding - 4) + bytes(padding - 4)
    archive.writestr(info, data, compresslevel=6)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--resources', type=Path, required=True)
    parser.add_argument('--dex', type=Path, required=True)
    parser.add_argument('--native-root', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with zipfile.ZipFile(args.output, 'w') as out:
        with zipfile.ZipFile(args.resources) as resources:
            for item in resources.infolist():
                # Keep AAPT2's choices; always enforce STORED on the resource table.
                method = zipfile.ZIP_STORED if item.filename == 'resources.arsc' else item.compress_type
                write_entry(out, item.filename, resources.read(item), method)
        write_entry(out, 'classes.dex', args.dex.read_bytes(), zipfile.ZIP_DEFLATED)
        for abi in ('arm64-v8a', 'x86_64'):
            # extractNativeLibs=true: compressed libraries are extracted by Android.
            native = args.native_root / abi / 'libpianoroom.so'
            write_entry(out, f'lib/{abi}/libpianoroom.so', native.read_bytes(), zipfile.ZIP_DEFLATED)
        for path in sorted(args.assets.rglob('*')):
            if path.is_file():
                relative = path.relative_to(args.assets)
                if any(part.startswith('.') for part in relative.parts) or path.name.endswith(('.partial', '.pending', '.tmp')):
                    continue
                if path.name.endswith('.sf2') and path.with_name(path.name+'.prsf').exists():
                    continue
                write_entry(out, 'assets/' + path.relative_to(args.assets).as_posix(), path.read_bytes(), zipfile.ZIP_DEFLATED)
    print(f'Unsigned APK: {args.output.stat().st_size} bytes')


if __name__ == '__main__':
    main()
