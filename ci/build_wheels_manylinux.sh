#!/bin/bash
# Usage: script.sh /path/to/qbdl /path/to/lief python_versions

set -ex

QBDL_ROOT=$1
shift
LIEF_DIR=$1
shift

export MAKEFLAGS="-j$(nproc)"

cd "$QBDL_ROOT/bindings/python"
for PY in $*; do
  /opt/python/$PY/bin/python ./setup.py --lief-dir=$LIEF_DIR bdist_wheel
  rm -rf build
done
for f in dist/*.whl; do
  auditwheel repair $f
done

# Tests

# Download a specific version of miasm
(cd /tmp && git clone https://github.com/cea-sec/miasm && cd miasm && git checkout 6285271a95f3ec9815728fb808a69bb21b50f770)

for PY in $*; do
  PYTHONBIN=/opt/python/$PY/bin/
  $PYTHONBIN/pip install pyparsing future wheelhouse/pyqbdl*$PY*.whl
  (cd /tmp/miasm && LDFLAGS="" $PYTHONBIN/python ./setup.py install && rm -rf build)
  $PYTHONBIN/python ./examples/miasm_macho_x64.py ../../examples/binaries/macho-x86-64-hello.bin |grep coucou
done
