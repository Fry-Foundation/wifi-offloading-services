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

        cp VERSION $RUN_DIR/VERSION

        # Run the collector program
        echo "Starting wayru-os-services collector in development mode..."
        echo "Available development scripts in dev/collector/scripts/:"
        echo "  - test-logs.sh: Generate test syslog messages"
        echo "  - collector.conf: Development configuration"
        echo ""
        cd $APP_DIR && ./collector --dev
        ;;

    *)
        echo "Error: Unknown app '$APP'"
        echo "Supported apps: agent, health, collector"
        exit 1
        ;;
esac
