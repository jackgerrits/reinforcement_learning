#!/bin/bash

set -e
set -x

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR=$SCRIPT_DIR/../../

cd $REPO_DIR/external_parser/build

# Run unit test suite with valgrind
valgrind --quiet --error-exitcode=100 --undef-value-errors=no --leak-check=full ./unit_tests/binary_parser_unit_tests
