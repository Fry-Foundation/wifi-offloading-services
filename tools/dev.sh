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

    "config")
        # Set up config app dev environment
        echo "Setting up config app in development environment..."
        mkdir -p dev/config/data

        cp VERSION dev/config/VERSION

        # Run the config program
        echo "Starting config app in development mode..."
        cd dev/config && ./config --dev
        ;;

    "collector")
        # Set up collector app dev environment
        echo "Setting up collector app development environment..."
        mkdir -p dev/collector/data
        mkdir -p dev/collector/logs
        mkdir -p dev/collector/scripts

        # Copy collector development scripts and config
        echo "Copying collector scripts and configuration files..."
        cp apps/collector/scripts/dev/* dev/collector/scripts/
        chmod +x dev/collector/scripts/*

        cp VERSION dev/collector/VERSION

        # Run the collector program
        echo "Starting wayru-os-services collector in development mode..."
        echo "Available development scripts in dev/collector/scripts/:"
        echo "  - test-logs.sh: Generate test syslog messages"
        echo "  - collector.conf: Development configuration"
        echo ""
        cd dev/collector && ./collector --dev
        ;;

    *)
        echo "Error: Unknown app '$APP'"
        echo "Supported apps: agent, health, collector"
        exit 1
        ;;
esac
