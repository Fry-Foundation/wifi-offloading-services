#!/bin/bash

if ! command -v bear &> /dev/null
then
    echo "bear could not be found. Please install bear."
    exit
fi

# Change to project root directory to find Makefile-dev.mk
cd "$(dirname "$0")/.."

bear -- make -f Makefile-dev.mk compile-only
