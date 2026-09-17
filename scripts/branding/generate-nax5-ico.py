#!/usr/bin/env python3
"""Regenerate gui/nax5.ico from the brand sheet app icon."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
IMPORT_SCRIPT = Path(__file__).resolve().parent / "import-nax5-logo.py"


def main() -> None:
    result = subprocess.run([sys.executable, str(IMPORT_SCRIPT)], check=False)
    if result.returncode != 0:
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
