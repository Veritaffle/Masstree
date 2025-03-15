#!/bin/bash

set -e
BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Directory '$BUILD_DIR' does not exist."
    exit 1
fi

cd "$BUILD_DIR"
for dir in */ ; do
    if [ -d "$dir" ]; then
        echo "making $dir..."
        cd "$dir"
        if ! make; then
            echo "Error: make failed in $dir"
            exit 1
        fi
        cd ..
    fi
done