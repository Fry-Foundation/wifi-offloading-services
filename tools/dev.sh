#!/usr/bin/env bash

set -e

# Copy scripts and data files
echo "Copying scripts and data files..."
mkdir -p dev/agent/scripts
cp apps/agent/scripts/dev/* dev/agent/scripts/
chmod +x dev/agent/scripts/*

mkdir -p dev/agent/data
mkdir -p dev/agent/data/did-key

cp VERSION dev/agent/VERSION

# Run the program
echo "Starting wayru-os-services in development mode..."
cd dev/agent && ./agent --dev
