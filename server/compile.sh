#!/bin/bash

OUTPUT_DIR="./output"
mkdir -p "$OUTPUT_DIR"

compile_c_files() {
    local CURRENT_DIR="$1"
    for item in "$CURRENT_DIR"/*; do
        if [ "$item" == "$OUTPUT_DIR" ]; then
            continue
        fi
        if [ -d "$item" ]; then
            compile_c_files "$item"
        elif [[ "$item" == *.c ]]; then
            rel_path="${item#./}"
            subdir="$OUTPUT_DIR/$(dirname "$rel_path")"
            mkdir -p "$subdir"
            exe_name=$(basename "$item" .c)
            arm-linux-gnueabihf-gcc "$item" -o "$subdir/$exe_name" -static
            echo "Compiled $item -> $subdir/$exe_name"
        fi
    done
}

compile_c_files "."

scp -O -r output/* root@192.168.1.100:/mnt/mmcblk0p1/www
