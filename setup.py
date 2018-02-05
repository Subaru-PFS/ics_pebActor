import distutils
from distutils.core import setup, Extension

import sdss3tools
import os

sdss3tools.setup(
    description = "PFI Electric Box",
    name = "ics_pebActor",
)
