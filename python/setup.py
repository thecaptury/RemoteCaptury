from setuptools import setup, Extension, find_packages
import os

# Define the extension module
# We need to access files from the parent directory.
# Since we are running setup.py from python/, the parent is ../
sources = [
    'src/bindings.cpp',
    '../RemoteCaptury.cpp'
]

module1 = Extension('_remotecaptury',
                    sources=sources,
                    include_dirs=['..'],
                    extra_compile_args=['-std=c++11'],
                    )

setup(name='remotecaptury',
      version='1.0.0',
      description='Python wrapper for RemoteCaptury',
      author='Captury',
      packages=find_packages(),
      ext_modules=[module1],
      zip_safe=False)
