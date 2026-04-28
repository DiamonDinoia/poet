#!/usr/bin/env python3
"""Generate Compiler Explorer URLs for examples/*.cpp.

For each example, this script builds a single-source CE clientstate URL where
`#include <poet/poet.hpp>` is rewritten to `#include <https://raw.githubusercontent.com/.../poet.hpp>`.
Compiler Explorer's web client expands such URL includes client-side via XHR
(see compiler-explorer/compiler-explorer#1442), inlining the amalgamated header
before compilation. POET ships a single-header build on the `single-header`
branch, which is the supported case.

With `--shorten`, the clientstate is POSTed to godbolt.org/api/shortener so
the JSON contains short `godbolt.org/z/<hash>` links suitable for the README.
The script writes `examples/godbolt_links.json` mapping "<example>:<compiler>"
-> URL, with one entry per (example, compiler) pair.
"""

from __future__ import annotations

import argparse
import base64
import json
import sys
import urllib.error
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
EXAMPLES_DIR = REPO_ROOT / "examples"
OUT_PATH = EXAMPLES_DIR / "godbolt_links.json"

# Files that embed Compiler Explorer shortlinks. When `--update-docs` is
# passed, any old `https://godbolt.org/z/<id>` URL that maps to a known
# example via the previous `godbolt_links.json` is rewritten to the new
# shortlink. Any new file that hosts CE links should be added here.
DOC_FILES = [
    REPO_ROOT / "README.md",
    REPO_ROOT / "examples" / "README.md",
    REPO_ROOT / "docs" / "index.rst",
    REPO_ROOT / "docs" / "guides" / "static_for.rst",
    REPO_ROOT / "docs" / "guides" / "dynamic_for.rst",
    REPO_ROOT / "docs" / "guides" / "dispatch.rst",
    REPO_ROOT / "docs" / "guides" / "benchmarks.rst",
]

DEFAULT_HEADER_URL = (
    "https://raw.githubusercontent.com/DiamonDinoia/poet/single-header/poet.hpp"
)

COMPILERS = {
    "gcc14": {
        "id": "g141",
        "options": "-std=c++20 -O3 -Wall -Wextra",
    },
}

# Per-example overrides keyed by file stem. Use to attach extra libraries,
# enable execute mode, append flags, etc. Anything omitted falls back to
# the defaults in `_make_state`.
EXAMPLE_OVERRIDES: "dict[str, dict]" = {
    "benchmark": {
        # Run on CE: compile + link Google Benchmark, switch to Execute view.
        "extra_options": "-pthread",
        "libs": [{"id": "benchmark", "version": "trunk"}],
        "execute": True,
    },
}

GODBOLT_BASE = "https://godbolt.org/clientstate/"
GODBOLT_SHORTEN = "https://godbolt.org/api/shortener"


def _encode_clientstate(state: dict) -> str:
    raw = json.dumps(state, separators=(",", ":")).encode("utf-8")
    return base64.urlsafe_b64encode(raw).decode("ascii").rstrip("=")


def _make_state(
    header_url: str,
    example_text: str,
    compiler_id: str,
    options: str,
    override: "dict | None" = None,
) -> dict:
    # Compiler Explorer's web client expands `#include <https://...>` by
    # fetching the URL via XHR (CORS-permitting host required) and inlining
    # the contents before compilation. Since POET ships a single amalgamated
    # header on the `single-header` branch, we just point the example at the
    # raw GitHub URL — no need to bundle the header in clientstate.
    # See: https://github.com/compiler-explorer/compiler-explorer/issues/1442
    rewritten = example_text.replace("<poet/poet.hpp>", f"<{header_url}>")

    override = override or {}
    full_options = options
    if "extra_options" in override:
        full_options = f"{options} {override['extra_options']}"
    libs = override.get("libs", [])
    execute = bool(override.get("execute", False))

    filters = {
        "binary": False,
        "execute": execute,
        "labels": True,
        "directives": True,
        "commentOnly": True,
        "trim": True,
        "libraryCode": False,
        "intel": True,
        "demangle": True,
    }

    compiler_pane = {
        "id": compiler_id,
        "options": full_options,
        "libs": libs,
        "tools": [],
        "filters": filters,
    }

    main_session = {
        "id": 1,
        "language": "c++",
        "source": rewritten,
        "filename": "main.cpp",
        "compilers": [compiler_pane],
        # In execute mode CE wires an executor pane to the same compiler.
        "executors": [
            {
                "compiler": {**compiler_pane, "filters": {"execute": True}},
                "compilerVisible": False,
                "compilerOutputVisible": False,
                "argsVisible": False,
                "argsPanelShown": False,
                "stdinVisible": False,
                "stdinPanelShown": False,
                "compilerFlagsVisible": False,
                "compilerFlagsPanelShown": False,
                "wrap": False,
            }
        ]
        if execute
        else [],
    }

    return {"sessions": [main_session]}


def _shorten(state: dict) -> str:
    payload = json.dumps(state).encode("utf-8")
    req = urllib.request.Request(
        GODBOLT_SHORTEN,
        data=payload,
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=30) as resp:
        body = json.loads(resp.read().decode("utf-8"))
    return body["url"]


def _canonical(state: dict) -> str:
    return json.dumps(state, sort_keys=True, separators=(",", ":"))


def _existing_short_matches(short_url: str, new_state: dict) -> bool:
    """Return True if `short_url` already resolves to a saved clientstate
    equivalent to `new_state` — used to keep stable shortlinks across
    regenerations when nothing actually changed."""
    if not short_url.startswith("https://godbolt.org/z/"):
        return False
    short_id = short_url.rsplit("/", 1)[-1]
    try:
        with urllib.request.urlopen(
            f"https://godbolt.org/api/shortlinkinfo/{short_id}", timeout=20
        ) as resp:
            current = json.loads(resp.read().decode("utf-8"))
    except (urllib.error.URLError, json.JSONDecodeError):
        return False
    return _canonical(current) == _canonical(new_state)


def _emit_redirects(links: "dict[str, str]", out_dir: Path) -> None:
    """Write a tiny `<example>.html` meta-refresh page per shortlink, plus
    an `index.html` listing them. The pages live on the `godbolt-links`
    branch (served via GitHub Pages) so README/docs URLs stay stable while
    the underlying godbolt.org/z/<id> targets rotate."""
    out_dir.mkdir(parents=True, exist_ok=True)
    template = (
        "<!doctype html>\n"
        '<html lang="en"><head>\n'
        '<meta charset="utf-8">\n'
        "<title>POET · {key} on Compiler Explorer</title>\n"
        '<meta http-equiv="refresh" content="0; url={url}">\n'
        '<link rel="canonical" href="{url}">\n'
        "</head><body>\n"
        '<p>Redirecting to <a href="{url}">{url}</a>…</p>\n'
        "</body></html>\n"
    )
    for key, url in sorted(links.items()):
        (out_dir / f"{key}.html").write_text(
            template.format(key=key, url=url), encoding="utf-8"
        )

    rows = "\n".join(
        f'  <li><a href="{key}.html">{key}</a> → <code>{url}</code></li>'
        for key, url in sorted(links.items())
    )
    index = (
        "<!doctype html>\n"
        '<html lang="en"><head><meta charset="utf-8">\n'
        "<title>POET · Compiler Explorer redirects</title></head>\n"
        "<body>\n"
        "<h1>POET — Compiler Explorer redirects</h1>\n"
        "<p>Each link below redirects to the current Compiler Explorer "
        "shortlink for that example.</p>\n"
        f"<ul>\n{rows}\n</ul>\n"
        "</body></html>\n"
    )
    (out_dir / "index.html").write_text(index, encoding="utf-8")


def _rewrite_doc_links(old_to_new: "dict[str, str]") -> "list[Path]":
    """Replace any old shortlink with the new one across DOC_FILES.
    Returns the list of files that changed."""
    if not old_to_new:
        return []
    changed: "list[Path]" = []
    for path in DOC_FILES:
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8")
        new_text = text
        for old, new in old_to_new.items():
            if old != new and old in new_text:
                new_text = new_text.replace(old, new)
        if new_text != text:
            path.write_text(new_text, encoding="utf-8")
            changed.append(path)
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--header-url",
        default=DEFAULT_HEADER_URL,
        help="Raw GitHub URL of the amalgamated single header.",
    )
    parser.add_argument(
        "--shorten",
        action="store_true",
        help="POST each URL to godbolt.org/api/shortener.",
    )
    parser.add_argument(
        "--update-docs",
        action="store_true",
        help="Rewrite shortlinks in README/docs in place.",
    )
    parser.add_argument("--out", default=str(OUT_PATH), help="Output JSON path.")
    parser.add_argument(
        "--existing-json",
        default=None,
        help="Path to a previous godbolt_links.json to seed "
        "idempotency checks (lets CI fetch the prior "
        "JSON from the godbolt-links branch).",
    )
    parser.add_argument(
        "--emit-redirects",
        default=None,
        metavar="DIR",
        help="Write per-example HTML meta-refresh pages (plus index.html) to DIR.",
    )
    args = parser.parse_args()

    examples = sorted(EXAMPLES_DIR.glob("*.cpp"))
    if not examples:
        print("error: no examples found", file=sys.stderr)
        return 1

    # Load the existing JSON so we can reuse stable shortlinks when the
    # underlying clientstate hasn't changed. CI passes --existing-json
    # pointing at a copy fetched from the godbolt-links branch.
    out_path = Path(args.out)
    existing_path = Path(args.existing_json) if args.existing_json else out_path
    existing: "dict[str, str]" = {}
    if existing_path.exists():
        try:
            existing = json.loads(existing_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            existing = {}

    out: dict[str, str] = {}
    reused = 0
    for example_path in examples:
        example_text = example_path.read_text(encoding="utf-8")
        for compiler_key, cfg in COMPILERS.items():
            state = _make_state(
                args.header_url,
                example_text,
                cfg["id"],
                cfg["options"],
                override=EXAMPLE_OVERRIDES.get(example_path.stem),
            )
            url = GODBOLT_BASE + _encode_clientstate(state)
            key = (
                example_path.stem
                if len(COMPILERS) == 1
                else f"{example_path.stem}:{compiler_key}"
            )

            if args.shorten:
                prior = existing.get(key)
                if prior and _existing_short_matches(prior, state):
                    url = prior
                    reused += 1
                else:
                    try:
                        url = _shorten(state)
                    except urllib.error.URLError as exc:
                        print(
                            f"warning: shorten failed for {key}: {exc}", file=sys.stderr
                        )
            out[key] = url

    out_path.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    if args.shorten:
        print(
            f"wrote {len(out)} URLs to {out_path} ({reused} reused, "
            f"{len(out) - reused} regenerated)"
        )
    else:
        print(f"wrote {len(out)} URLs to {out_path}")

    if args.update_docs:
        old_to_new = {
            existing[k]: out[k] for k in out if k in existing and existing[k] != out[k]
        }
        changed = _rewrite_doc_links(old_to_new)
        if changed:
            print(f"updated {len(changed)} doc file(s):")
            for p in changed:
                print(f"  {p.relative_to(REPO_ROOT)}")
        else:
            print("doc files already up to date")

    if args.emit_redirects:
        redirect_dir = Path(args.emit_redirects)
        _emit_redirects(out, redirect_dir)
        print(f"wrote {len(out) + 1} HTML pages to {redirect_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
