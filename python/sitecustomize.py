"""Bootstrap import behavior for local source-tree usage.

This file is copied into the build directory so that `import remotecaptury`
resolves to the Python package wrapper instead of the compiled extension module
when the build directory is on PYTHONPATH.
"""

import importlib.util
import pathlib
import sys


def _install_package_alias():
    build_dir = pathlib.Path(__file__).resolve().parent
    package_dir = build_dir.parent / "python" / "remotecaptury"
    if not package_dir.exists():
        return

    # Only install if the compiled extension module is shadowing the package name.
    if "remotecaptury" in sys.modules:
        return

    spec = importlib.util.spec_from_file_location("remotecaptury", package_dir / "__init__.py")
    if spec is None or spec.loader is None:
        return
    module = importlib.util.module_from_spec(spec)
    sys.modules["remotecaptury"] = module
    spec.loader.exec_module(module)


_install_package_alias()
