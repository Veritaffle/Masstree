#!/bin/bash

set -e
BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Directory '$BUILD_DIR' does not exist."
    exit 1
fi

cd "$BUILD_DIR"

# ./make-all.sh clean
if [ "$1" == "clean" ]; then
    for dir in */ ; do
        if [ -d "$dir" ]; then
            echo "Cleaning $dir..."
            cd "$dir"
            if ! make clean; then
                echo "Error: make clean failed in $dir"
                exit 1
            fi
            cd ..
        fi
    done
    echo "Cleanup complete."
    exit 0
fi

# ./make-all.sh
for dir in */ ; do
    if [ -d "$dir" ]; then
        echo "Making $dir..."
        cd "$dir"
        if ! make; then
            echo "Error: make failed in $dir"
            exit 1
        fi
        cd ..
    fi
done