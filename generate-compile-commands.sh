#!/bin/bash

if ! command -v bear &> /dev/null
then
    echo "bear could not be found. Please install bear."
    exit
fi

bear -- make -f Makefile-dev.mk compile-only
