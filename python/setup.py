import re
from pathlib import Path
from setuptools import setup, Extension, find_packages

ROOT = Path(__file__).resolve().parent
text = (ROOT / 'remotecaptury' / '__init__.py').read_text(encoding='utf-8')
match = re.search(r'^__version__\s*=\s*["\']([^"\']+)["\']', text, re.MULTILINE)
if match is None:
    raise RuntimeError('Could not determine package version from remotecaptury/__init__.py')
package_version = match.group(1)

# Define the extension module
sources = [
    'src/RemoteCapturyPython.cpp',
    'src/RemoteCaptury.cpp'
]

module1 = Extension('_remotecaptury',
                    sources=sources,
                    include_dirs=['src'],
                    extra_compile_args=['-std=c++11'],
                    )

setup(name='remotecaptury',
      version=package_version,
      description='Python wrapper for RemoteCaptury',
      author='Captury',
      license='BSD-2-Clause',
      python_requires='>=3.6',
      packages=find_packages(),
      ext_modules=[module1],
      zip_safe=False)
