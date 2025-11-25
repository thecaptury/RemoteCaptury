#!/bin/bash
# Script to prepare the package for building by copying necessary C++ files

set -e

echo "Copying C++ source files from parent directory..."

# Copy C++ files
cp ../RemoteCaptury.cpp src/
cp ../RemoteCaptury.h src/

# Copy captury directory
cp -r ../captury src/

echo "✓ Files copied successfully"
echo ""
echo "You can now build the package with:"
echo "  python -m build"
