#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/.build"
DEST_DIR="${SCRIPT_DIR}/nil_gate"

# Verify build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo "Error: Build directory not found at ${BUILD_DIR}"
    exit 1
fi

# Copy the C API library
if [ ! -f "${BUILD_DIR}/out/lib/libnil-gate-c-api.so" ]; then
    echo "Error: libnil-gate-c-api.so not found at ${BUILD_DIR}/out/lib/"
    exit 1
fi

cp "${BUILD_DIR}/out/lib/libnil-gate-c-api.so" "${DEST_DIR}/libnil-gate-c-api.so"
echo "✓ Copied libnil-gate-c-api.so to ${DEST_DIR}"

# Install build dependencies
cd "${SCRIPT_DIR}"
pip3 install --quiet build auditwheel
echo "✓ Installed build dependencies"

# Build wheel
python3 -m build --wheel
echo "✓ Built wheel package"

# Repair wheel for broader Linux compatibility
mkdir -p dist
auditwheel repair dist/*.whl --wheel-dir dist/
echo "✓ Repaired wheel for manylinux compatibility"

echo ""
echo "Build complete! Wheels are in ${SCRIPT_DIR}/dist/"
echo ""
echo "To upload to PyPI, run:"
echo "  pip3 install twine"
echo "  python3 -m twine upload dist/*