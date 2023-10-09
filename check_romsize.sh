#!/bin/bash

SIZE=$(wc -c < $1)
SET_ROM_SIZE=$(grep -Po '^\s*u32 romSize = \K[0-9]+' < source/main.c)
echo "Rom size: ${SIZE}!"
if (($SIZE > $SET_ROM_SIZE)); then
	echo "!!!!!!Rom size set in code is smaller than the actual size of the Rom!!!!!"
	echo "check romSize in source/main.c"
	exit 1
fi
