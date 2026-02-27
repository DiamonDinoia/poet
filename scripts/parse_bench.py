#!/usr/bin/env python3
"""Parse nanobench text table output into structured JSON.

Usage:
    python3 parse_bench.py INPUT.txt OUTPUT.json
    python3 parse_bench.py INPUT.txt  # prints to stdout

Handles:
- Multiple benchmark tables (each with a title line)
- relative(true) tables (extra relative_pct column)
- Register-info preamble (=== ... === blocks) as metadata
- Stability warnings from nanobench
"""

import json
import re
import sys
from pathlib import Path


def parse_bench_file(text: str) -> dict:
    """Parse a nanobench text output into structured data."""
    result = {
        "metadata": {},
        "tables": [],
        "warnings": [],
    }

    lines = text.splitlines()
    i = 0

    # Parse metadata preamble (=== Title === blocks and key: value lines)
    while i < len(lines):
        line = lines[i].strip()
        if not line:
            i += 1
            continue
        # Metadata section header: === Some Title ===
        m = re.match(r"^===\s*(.+?)\s*===$", line)
        if m:
            section = m.group(1)
            result["metadata"]["_section"] = section
            i += 1
            continue
        # Key: value metadata (allow parens/hyphens in key names)
        m = re.match(r"^([\w][\w\s()\-]*?):\s+(.+)$", line)
        if m and not line.startswith("|") and "ns/op" not in line:
            result["metadata"][m.group(1).strip()] = m.group(2).strip()
            i += 1
            continue
        # Once we hit a table header or separator, stop metadata parsing
        break

    # Parse tables
    current_title = None
    header_cols = None
    rows = []

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Nanobench warning/recommendation lines
        if (
            stripped.startswith("Warning")
            or "stability" in stripped.lower()
            or stripped.startswith("Recommendations")
            or stripped.startswith("* ")
            or stripped.startswith("See ")
        ):
            result["warnings"].append(stripped)
            i += 1
            continue

        # Empty line: flush current table
        if not stripped:
            if header_cols and rows:
                result["tables"].append(
                    {
                        "title": current_title or "",
                        "columns": header_cols,
                        "rows": rows,
                    }
                )
                header_cols = None
                rows = []
            i += 1
            continue

        # Table separator (all dashes/pipes/colons)
        if re.match(r"^[\s|:\-+]+$", stripped) and len(stripped) > 3:
            i += 1
            continue

        # Table header line containing ns/op
        if "ns/op" in stripped and "|" in stripped:
            # Parse column headers
            parts = [p.strip() for p in stripped.split("|")]
            parts = [p for p in parts if p]  # remove empty from leading/trailing |
            # nanobench embeds the table title as the last column header
            # (the benchmark name column). Extract it as the title.
            if parts and not any(
                kw in parts[-1].lower() for kw in ["ns/op", "op/s", "err%", "cyc/op"]
            ):
                current_title = parts[-1]
            header_cols = parts
            rows = []
            i += 1
            continue

        # Table title line (no | separator, not metadata)
        if "|" not in stripped and not stripped.startswith("Warning"):
            # Could be a table title
            if header_cols and rows:
                # Flush previous table
                result["tables"].append(
                    {
                        "title": current_title or "",
                        "columns": header_cols,
                        "rows": rows,
                    }
                )
                header_cols = None
                rows = []
            current_title = stripped
            i += 1
            continue

        # Data row
        if "|" in stripped and header_cols:
            parts = [p.strip() for p in stripped.split("|")]
            parts = [p for p in parts if p]

            if len(parts) >= 2:
                row = {}
                for ci, col in enumerate(header_cols):
                    if ci < len(parts):
                        val = parts[ci].strip()
                        # Try to parse numeric values
                        cleaned = val.replace(",", "").replace("%", "").strip()
                        try:
                            if "." in cleaned:
                                row[col] = float(cleaned)
                            else:
                                row[col] = int(cleaned)
                        except ValueError:
                            row[col] = val
                    else:
                        row[col] = None
                rows.append(row)

            i += 1
            continue

        i += 1

    # Flush final table
    if header_cols and rows:
        result["tables"].append(
            {
                "title": current_title or "",
                "columns": header_cols,
                "rows": rows,
            }
        )

    return result


def main():
    if len(sys.argv) < 2:
        print("Usage: parse_bench.py INPUT.txt [OUTPUT.json]", file=sys.stderr)
        sys.exit(1)

    input_path = Path(sys.argv[1])
    text = input_path.read_text()
    parsed = parse_bench_file(text)

    if len(sys.argv) >= 3:
        output_path = Path(sys.argv[2])
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json.dumps(parsed, indent=2))
    else:
        print(json.dumps(parsed, indent=2))


if __name__ == "__main__":
    main()
