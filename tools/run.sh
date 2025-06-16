#!/usr/bin/env bash

set -e

APP="${1:-agent}"
RUN_DIR="run"
APP_DIR="$RUN_DIR/$APP"

echo "Setting up development environment for app: $APP"

case "$APP" in
    "agent")
        # Copy scripts and data files for agent
        echo "Copying agent scripts and data files..."
        mkdir -p $APP_DIR/scripts
        cp apps/agent/scripts/dev/* $APP_DIR/scripts/
        chmod +x $APP_DIR/scripts/*

        mkdir -p $APP_DIR/data
        mkdir -p $APP_DIR/data/did-key

        cp VERSION $RUN_DIR/VERSION

        # Run the agent program
        echo "Starting wayru-os-services agent in development mode..."
        cd $APP_DIR && ./agent --dev
        ;;

    "config")
        # Set up config app dev environment
        echo "Setting up config app in development environment..."
        mkdir -p $APP_DIR/data

        cp VERSION $RUN_DIR/VERSION

        # Run the config program
        echo "Starting config app in development mode..."
        cd $APP_DIR && ./config --dev
        ;;

    "collector")
        # Set up collector app dev environment
        echo "Setting up collector app development environment..."
        mkdir -p $APP_DIR/data
        mkdir -p $APP_DIR/logs
        mkdir -p $APP_DIR/scripts

        # Copy collector development scripts and config
        echo "Copying collector scripts and configuration files..."
        cp apps/collector/scripts/dev/* $APP_DIR/scripts/
        chmod +x $APP_DIR/scripts/*

        # Copy the collector development configuration file to the working directory
        echo "Copying collector development configuration file..."
        cp apps/collector/scripts/dev/wayru-collector.config $APP_DIR/
        echo "Configuration file: wayru-collector.config (optimized for local development)"

        cp VERSION $RUN_DIR/VERSION

        # Run the collector program
        echo "Starting wayru-os-services collector in development mode..."
        echo ""
        echo "Configuration:"
        echo "  - wayru-collector.config: UCI-style configuration (local development settings)"
        echo "  - Endpoint: http://localhost:8080/v1/logs (for mock backend)"
        echo "  - Batch size: 5 logs (fast testing)"
        echo "  - Queue size: 50 entries (small memory footprint)"
        echo ""
        echo "Available development scripts in run/collector/scripts/:"
        echo "  - test-logs.sh: Generate test syslog messages"
        echo "  - mock-backend.py: Local HTTP server for testing"
        echo "  - README.md: Development guide and documentation"
        echo ""
        echo "Quick start:"
        echo "  1. Start mock backend: python3 scripts/mock-backend.py --verbose"
        echo "  2. Generate test logs: ./scripts/test-logs.sh 10 1 normal"
        echo "  3. Monitor collector output for batch processing"
        echo ""
        cd $APP_DIR && ./collector --dev
        ;;

    *)
        echo "Error: Unknown app '$APP'"
        echo "Supported apps: agent, health, collector"
        exit 1
        ;;
esac
