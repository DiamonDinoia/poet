import textwrap
import os
import sys
import glob

# Project information
project = 'POET'
copyright = '2025, POET Authors'
author = 'Marco Barbone'

# Extensions
extensions = [
    'breathe',
    'exhale',
    'myst_parser',
    'sphinx_rtd_theme',
]

# Breathe configuration
# Use environment variable if available (set by CMake), otherwise default to build/
doxygen_xml = os.environ.get("DOXYGEN_XML_OUTPUT", "../build/docs/xml")
breathe_projects = {
    "POET": os.path.abspath(doxygen_xml)
}
breathe_default_project = "POET"

# Exhale configuration
exhale_args = {
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "doxygenStripFromPath":  os.path.abspath(".."),
    "rootFileTitle":         "POET API Reference",
    "createTreeView":        True,
    "exhaleExecutesDoxygen": False,
    # Keep generated pages focused on public API.
    "listingExclude": [
        r".*::detail::.*",
        r".*\\bdetail\\b.*",
        r".*DispatchSet::convert_tuple.*",
        r".*DispatchSet::seq_len.*",
        r"^poet::dispatch$",
        r"^poet::dispatch_tuples$",
        r"^poet::static_for$",
    ],
    # Avoid brittle overload resolution pages from generated function entries.
    "unabridgedOrphanKinds": [
        "dir",
        "namespace",
        "class",
        "struct",
        "enum",
        "typedef",
        "variable",
    ],
}

# Theme
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
}

# Breathe/Exhale emit false-positive warnings for heavily-overloaded templated
# APIs (notably dispatch/static_for) due to parser limitations.
suppress_warnings = [
    "docutils",
    "cpp.duplicate_declaration",
    "duplicate_declaration.cpp",
]


def _strip_private_macro_refs(app, env, docnames):
    api_dir = os.path.join(os.path.dirname(__file__), "api")
    if not os.path.isdir(api_dir):
        return

    blocked = (
        "``poet/core/macros.hpp``",
        "``poet/core/undef_macros.hpp``",
    )

    for rst_path in glob.glob(os.path.join(api_dir, "file_include_*.rst")):
        with open(rst_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        filtered = [line for line in lines if not any(token in line for token in blocked)]
        if filtered != lines:
            with open(rst_path, "w", encoding="utf-8") as f:
                f.writelines(filtered)


def setup(app):
    app.connect("env-before-read-docs", _strip_private_macro_refs)
