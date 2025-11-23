from setuptools import setup, Extension, find_packages
import os

# Define the extension module
sources = [
    'src/bindings.cpp',
    'src/RemoteCaptury.cpp'
]

module1 = Extension('_remotecaptury',
                    sources=sources,
                    include_dirs=['src'],
                    extra_compile_args=['-std=c++11'],
                    )

setup(name='remotecaptury',
      version='1.0.0',
      description='Python wrapper for RemoteCaptury',
      author='Captury',
      packages=find_packages(),
      ext_modules=[module1],
      zip_safe=False)
