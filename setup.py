#!/usr/bin/env python
"""Install this package. Requires sdss3tools.

To use:
python setup.py install
"""
import sdss3tools

sdss3tools.setup(
    description = "PFI Electric Box (EBox) interface",
    name = "ics_pebActor",
    install_lib="$base/python",
)
