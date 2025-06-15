#!/usr/bin/env bash

set -e

APP="${1:-agent}"

echo "Setting up development environment for app: $APP"

case "$APP" in
    "agent")
        # Copy scripts and data files for agent
        echo "Copying agent scripts and data files..."
        mkdir -p dev/agent/scripts
        cp apps/agent/scripts/dev/* dev/agent/scripts/
        chmod +x dev/agent/scripts/*

        mkdir -p dev/agent/data
        mkdir -p dev/agent/data/did-key

        cp VERSION dev/agent/VERSION

        # Run the agent program
        echo "Starting wayru-os-services agent in development mode..."
        cd dev/agent && ./agent --dev
        ;;

    "health")
        # Set up health app dev environment
        echo "Setting up health app development environment..."
        mkdir -p dev/health/data

        cp VERSION dev/health/VERSION

        # Run the health program
        echo "Starting wayru-os-services health in development mode..."
        cd dev/health && ./health --dev
        ;;

    *)
        echo "Error: Unknown app '$APP'"
        echo "Supported apps: agent, health"
        exit 1
        ;;
esac
