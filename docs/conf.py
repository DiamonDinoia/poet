import textwrap
import os
import sys

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
    "doxygenStripFromPath":  "..",
    "rootFileTitle":         "POET API Reference",
    "createTreeView":        True,
    "exhaleExecutesDoxygen": False
}

# Theme
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
}
