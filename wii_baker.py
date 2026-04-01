#!/usr/bin/env python3
import argparse
import os
import shutil
import struct
import subprocess
from dataclasses import dataclass
from pathlib import Path
import tempfile


@dataclass
class PFItem:
    name: str
    nxt: int
    file_folder: int
    offset: int = 0
    length: int = 0


@dataclass
class PFFileEntry:
    path: str
    offset: int
    length: int


def flip_32bit(data: bytes) -> bytes:
    chunk_len = (len(data) // 4) * 4
    if chunk_len == 0:
        return data
    swapped = struct.pack(">" + "I" * (chunk_len // 4), *struct.unpack("<" + "I" * (chunk_len // 4), data[:chunk_len])
    )
    return swapped + data[chunk_len:]


def flip_16bit(data: bytes) -> bytes:
    chunk_len = (len(data) // 2) * 2
    if chunk_len == 0:
        return data
    swapped = struct.pack(">" + "H" * (chunk_len // 2), *struct.unpack("<" + "H" * (chunk_len // 2), data[:chunk_len])
    )
    return swapped + data[chunk_len:]


def read_cstring_from_file(fp, pos: int, limit: int):
    fp.seek(pos)
    out = bytearray()
    while pos < limit:
        b = fp.read(1)
        if not b:
            break
        pos += 1
        if b == b"\x00":
            break
        out.extend(b)
    return out.decode("latin-1", errors="ignore"), pos


def read_le_u32(fp):
    data = fp.read(4)
    if len(data) != 4:
        raise EOFError
    return struct.unpack("<I", data)[0]


def parse_pf_items(pf_path: Path):
    size = pf_path.stat().st_size
    if size < 8:
        return []

    with pf_path.open("rb") as fp:
        fp.seek(size - 4)
        from_off = read_le_u32(fp)
        if from_off >= size - 4:
            return []

        fp.seek(from_off)
        elem_count = read_le_u32(fp)
        pos = from_off + 4
        items = []
        max_items = elem_count + 4096

        while pos < size - 4 and len(items) <= max_items:
            name, pos = read_cstring_from_file(fp, pos, size - 4)
            if pos + 8 > size:
                break

            fp.seek(pos)
            try:
                nxt = read_le_u32(fp)
                file_folder = read_le_u32(fp)
            except EOFError:
                break
            pos += 8

            if file_folder == 0xFFFFFFFF:
                if pos + 10 > size:
                    break
                fp.seek(pos)
                try:
                    offset = read_le_u32(fp)
                    length = read_le_u32(fp)
                except EOFError:
                    break
                pos += 10
                items.append(PFItem(name=name, nxt=nxt, file_folder=file_folder, offset=offset, length=length))
            else:
                items.append(PFItem(name=name, nxt=nxt, file_folder=file_folder))

    return items


def collect_files(items):
    if not items:
        return []

    out = []
    dir_stack = []
    visited_dirs = set()

    root_head = items[0].nxt if items[0].name == "." else 0
    dir_stack.append((root_head, ""))

    while dir_stack:
        head, prefix = dir_stack.pop()
        if head == 0xFFFFFFFF or head >= len(items):
            continue

        visit_key = (head, prefix)
        if visit_key in visited_dirs:
            continue
        visited_dirs.add(visit_key)

        idx = head
        guard = 0
        while idx < len(items) and guard < len(items) + 2048:
            guard += 1
            it = items[idx]
            name = it.name.strip("/\\")

            if it.file_folder == 0xFFFFFFFF:
                if name:
                    full = f"{prefix}/{name}" if prefix else name
                    out.append(PFFileEntry(path=full.replace("\\", "/"), offset=it.offset, length=it.length))
            else:
                if name and name != ".":
                    child_prefix = f"{prefix}/{name}" if prefix else name
                    dir_stack.append((it.file_folder, child_prefix.replace("\\", "/")))

            if it.nxt == 0xFFFFFFFF or it.nxt == idx:
                break
            idx = it.nxt

    dedup = {}
    for ent in out:
        dedup[ent.path] = ent
    return sorted(dedup.values(), key=lambda x: x.path)


def stream_copy(fp, offset: int, length: int, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as wf:
        fp.seek(offset)
        remain = length
        while remain > 0:
            chunk = fp.read(min(1024 * 1024, remain))
            if not chunk:
                break
            wf.write(chunk)
            remain -= len(chunk)


def stream_flip_16(fp, offset: int, length: int, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as wf:
        fp.seek(offset)
        remain = length
        carry = b""
        while remain > 0:
            chunk = fp.read(min(1024 * 1024, remain))
            if not chunk:
                break
            remain -= len(chunk)
            chunk = carry + chunk
            usable = (len(chunk) // 2) * 2
            body = chunk[:usable]
            carry = chunk[usable:]
            if body:
                cnt = len(body) // 2
                wf.write(struct.pack(">" + "H" * cnt, *struct.unpack("<" + "H" * cnt, body)))
        if carry:
            wf.write(carry)


def stream_flip_32(fp, offset: int, length: int, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as wf:
        fp.seek(offset)
        remain = length
        carry = b""
        while remain > 0:
            chunk = fp.read(min(1024 * 1024, remain))
            if not chunk:
                break
            remain -= len(chunk)
            chunk = carry + chunk
            usable = (len(chunk) // 4) * 4
            body = chunk[:usable]
            carry = chunk[usable:]
            if body:
                cnt = len(body) // 4
                wf.write(struct.pack(">" + "I" * cnt, *struct.unpack("<" + "I" * cnt, body)))
        if carry:
            wf.write(carry)


def maybe_convert_texture(fp, offset: int, length: int, src_ext: str, out_path: Path, tpl_conv: str):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    tpl_path = out_path.with_suffix(".tpl")
    with tempfile.NamedTemporaryFile(delete=False, suffix=src_ext) as tmp:
        tmp_path = Path(tmp.name)
        fp.seek(offset)
        remain = length
        while remain > 0:
            chunk = fp.read(min(1024 * 1024, remain))
            if not chunk:
                break
            tmp.write(chunk)
            remain -= len(chunk)

    try:
        subprocess.run([tpl_conv, "-i", str(tmp_path), "-o", str(tpl_path), "-f", "CMPR"], check=True)
    finally:
        if tmp_path.exists():
            tmp_path.unlink()


def bake_entry(fp, entry: PFFileEntry, out_root: Path, tpl_conv: str):
    clean_name = entry.path.lstrip("/\\").replace("\\", "/")
    out_path = out_root / clean_name
    ext = out_path.suffix.lower()

    if ext in {".vtx", ".msh", ".phy", ".de2"}:
        stream_flip_32(fp, entry.offset, entry.length, out_path)
    elif ext in {".idx"}:
        stream_flip_16(fp, entry.offset, entry.length, out_path)
    elif ext in {".tga", ".bmp"}:
        try:
            maybe_convert_texture(fp, entry.offset, entry.length, ext, out_path, tpl_conv)
        except Exception:
            stream_copy(fp, entry.offset, entry.length, out_path)
    else:
        stream_copy(fp, entry.offset, entry.length, out_path)


def bake_pf(pf_path: Path, output_dir: Path, tpl_conv: str):
    size = pf_path.stat().st_size
    items = parse_pf_items(pf_path)
    files = collect_files(items)
    print(f"[WII_BAKER] {pf_path}: items={len(items)} files={len(files)}")

    with pf_path.open("rb") as fp:
        for ent in files:
            if ent.offset + ent.length > size:
                continue
            print(f"[WII_BAKER] Baking: {ent.path}")
            bake_entry(fp, ent, output_dir, tpl_conv)


def main():
    parser = argparse.ArgumentParser(description="Bake .pf assets to Wii loose files")
    parser.add_argument("pf_files", nargs="+", help="Input .pf files")
    parser.add_argument("--output", default="./WII_DATA", help="Output dir (default: ./WII_DATA)")
    parser.add_argument("--tpl-conv", default="img2tpl", help="Texture converter executable")
    parser.add_argument("--clean", action="store_true", help="Delete output dir before baking")
    args = parser.parse_args()

    output_dir = Path(args.output)
    if args.clean and output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    for pf in args.pf_files:
        bake_pf(Path(pf), output_dir, args.tpl_conv)


if __name__ == "__main__":
    main()
