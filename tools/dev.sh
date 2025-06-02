#!/usr/bin/env bash

set -e

# Copy scripts and data files
echo "Copying scripts and data files..."
mkdir -p output/scripts
cp source/scripts/dev/* output/scripts/
chmod +x output/scripts/*

mkdir -p output/data
mkdir -p output/data/did-key

cp VERSION output/VERSION

# Run the program
echo "Starting wayru-os-services in development mode..."
cd output && ./wayru-os-services --dev 