# fvlab_env.py - finds fvlab.exe (env FVLAB_BIN or --fvlab) and the TMUF Packs folder (env TM_PACKS or --packs) for the scripts here. MIT licence, see LICENSE-MIT.
import os
import sys

DEFAULT_PACKS = r"C:\Program Files (x86)\TmUnitedForever\Packs"


def fvlab_exe(override=None):
    """Path of fvlab.exe: the argument, else FVLAB_BIN. Build it from tools/fvlab/README.md."""
    path = override or os.environ.get("FVLAB_BIN")
    if not path or not os.path.isfile(path):
        sys.exit("fvlab.exe not found: set FVLAB_BIN to its full path (build: tools/fvlab/README.md) or pass --fvlab")
    return path


def packs_dir(override=None):
    """Packs folder of a complete TMUF install: the argument, else TM_PACKS, else the default install path."""
    path = override or os.environ.get("TM_PACKS") or DEFAULT_PACKS
    if not os.path.isdir(path):
        sys.exit(f"Packs folder not found: {path} (set TM_PACKS to <TMUF install>\\Packs)")
    return path
