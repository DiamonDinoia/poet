#!/usr/bin/env python3
# python
"""
Amalgamate local headers into a single header.

Usage:
  tools/amalgamate.py [--root ROOT] [--input PATH] [--output PATH] [--guard MACRO]

Defaults:
  root:   .
  input:  include/poet/poet.hpp
  output: include/poet/poet.gold.hpp
  guard:  POET_SINGLE_HEADER_GOLDBOT_HPP
"""
from pathlib import Path
import argparse
import re
import sys

INCLUDE_Q_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"\s*$')
INCLUDE_LT_RE = re.compile(r'^\s*#\s*include\s*<([^>]+)>\s*$')
IFNDEF_RE = re.compile(r'^\s*#\s*ifndef\s+([A-Z0-9_]+)\s*$')
DEFINE_RE = re.compile(r'^\s*#\s*define\s+([A-Z0-9_]+)\s*$')
ENDIF_RE = re.compile(r'^\s*#\s*endif\b')
PRAGMA_ONCE_RE = re.compile(r'^\s*#\s*pragma\s+once\b')

def read_text(path: Path) -> str:
    return path.read_text(encoding='utf-8')

def strip_header_guard(text: str) -> str:
    """
    Remove leading conventional header guard (#ifndef / #define) and trailing #endif,
    and remove '#pragma once' lines.
    """
    lines = text.splitlines()
    # Remove pragma once
    lines = [ln for ln in lines if not PRAGMA_ONCE_RE.match(ln)]
    guard = None
    guard_idx = None
    for i in range(min(8, max(0, len(lines)-1))):
        m1 = IFNDEF_RE.match(lines[i])
        if m1:
            m2 = DEFINE_RE.match(lines[i+1]) if i+1 < len(lines) else None
            if m2 and m1.group(1) == m2.group(1):
                guard = m1.group(1)
                guard_idx = i
                break
    if guard and guard_idx is not None:
        # remove the #ifndef and #define
        del lines[guard_idx:guard_idx+2]
        # remove the last #endif
        for j in range(len(lines)-1, -1, -1):
            if ENDIF_RE.match(lines[j]):
                del lines[j]
                break
    return "\n".join(lines) + ("\n" if lines and not lines[-1].endswith("\n") else "")

def find_project_header(name: str, current_file: Path, root: Path, include_roots=None) -> Path:
    """
    Try to resolve an include name to a project header file.
    Search order: current dir, include_roots (e.g. root/include), then provided extra include_roots.
    Returns Path or None.
    """
    include_roots = include_roots or [root / "include"]
    cur_dir = current_file.parent
    candidates = [cur_dir] + include_roots
    for d in candidates:
        p = (d / name).resolve()
        try:
            p.relative_to(root)
            inside = True
        except Exception:
            inside = False
        if inside and p.exists():
            return p
    return None

def inline_file(path: Path, root: Path, processed: set, include_roots=None) -> str:
    path = path.resolve()
    if str(path) in processed:
        return f"/* Skipped already inlined: {path.relative_to(root)} */\n"
    processed.add(str(path))

    try:
        text = read_text(path)
    except Exception as e:
        raise

    text = strip_header_guard(text)
    out_lines = []
    out_lines.append(f"// BEGIN_FILE: {path.relative_to(root)}\n")
    for raw in text.splitlines():
        line = raw
        m_q = INCLUDE_Q_RE.match(line)
        if m_q:
            inc = m_q.group(1)
            inc_path = find_project_header(inc, path, root, include_roots)
            if inc_path:
                out_lines.append(f"/* Begin inline (quoted): {inc_path.relative_to(root)} */\n")
                out_lines.append(inline_file(inc_path, root, processed, include_roots))
                out_lines.append(f"/* End inline (quoted): {inc_path.relative_to(root)} */\n")
                continue
            else:
                # leave unchanged (could be non-project path)
                out_lines.append(line + "\n")
                continue

        m_lt = INCLUDE_LT_RE.match(line)
        if m_lt:
            inc = m_lt.group(1)
            inc_path = find_project_header(inc, path, root, include_roots)
            if inc_path:
                out_lines.append(f"/* Begin inline (angle): {inc_path.relative_to(root)} */\n")
                out_lines.append(inline_file(inc_path, root, processed, include_roots))
                out_lines.append(f"/* End inline (angle): {inc_path.relative_to(root)} */\n")
                continue
            else:
                # system header: preserve
                out_lines.append(line + "\n")
                continue

        # normal line
        out_lines.append(line + ("\n" if not line.endswith("\n") else ""))
    out_lines.append(f"// END_FILE: {path.relative_to(root)}\n")
    return "".join(out_lines)

def build_single_header(input_path: Path, output_path: Path, root: Path, guard_macro: str):
    processed = set()
    body = inline_file(input_path.resolve(), root.resolve(), processed, include_roots=[root / "include"])
    header_comment = (
        "/* Auto-generated single-header. Do not edit directly. */\n\n"
    )
    guard = guard_macro or "SINGLE_HEADER_GOLDBOT_HPP"
    header = f"#ifndef {guard}\n#define {guard}\n\n"
    footer = f"\n#endif // {guard}\n"
    content = header_comment + header + body + footer
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(content, encoding="utf-8")
    print(f"Wrote {output_path}")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--root", "-r", default=".")
    p.add_argument("--input", "-i", default="include/poet/poet.hpp")
    p.add_argument("--output", "-o", default="include/poet/poet.gold.hpp")
    p.add_argument("--guard", default="POET_SINGLE_HEADER_GOLDBOT_HPP")
    args = p.parse_args()

    root = Path(args.root).resolve()
    inp = (root / args.input).resolve()
    out = (root / args.output).resolve()

    if not inp.exists():
        print(f"Input header not found: {inp}", file=sys.stderr)
        sys.exit(2)

    build_single_header(inp, out, root, args.guard)

if __name__ == "__main__":
    main()
