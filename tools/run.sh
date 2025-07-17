#!/usr/bin/env bash

set -e

APP="${1:-agent}"
RUN_DIR="run"
APP_DIR="$RUN_DIR/wayru-$APP"

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
        cd $APP_DIR && ./wayru-agent --dev
        ;;

    "config")
        # Set up config app dev environment
        echo "Setting up config app in development environment..."        
        mkdir -p $APP_DIR/scripts/dev
        mkdir -p $APP_DIR/scripts/dev/hashes

        # ✅ ARREGLADO: Copiar manteniendo la estructura de subdirectorios
        echo "Copying config scripts maintaining directory structure..."
        cp -r apps/config/scripts/dev/* $APP_DIR/scripts/dev/ 2>/dev/null || echo "No dev files to copy"
        chmod +x $APP_DIR/scripts/dev/* 2>/dev/null || echo "No scripts to make executable"

        # Crear directorio de hashes
        echo "Created hashes directory: $APP_DIR/scripts/dev/hashes"

        cp VERSION $RUN_DIR/VERSION

        # Run the config program
        echo "Starting config app in development mode..."
        cd $APP_DIR && ./wayru-config --dev
        ;;

    "collector")
        # Set up collector app dev environment
        echo "Setting up collector app development environment..."
        mkdir -p $APP_DIR/data
        mkdir -p $APP_DIR/logs
        mkdir -p $APP_DIR/scripts

        # Copy collector development scripts and config
        cp apps/collector/scripts/dev/* $APP_DIR/scripts/
        chmod +x $APP_DIR/scripts/*

        # Copy the collector development configuration file to the working directory
        echo "Copying collector development configuration file..."
        cp apps/collector/scripts/dev/wayru-collector.config $APP_DIR/
        echo "Configuration file: wayru-collector.config (optimized for local development)"

        cp VERSION $RUN_DIR/VERSION

        # Run the collector program
        cd $APP_DIR && ./wayru-collector --dev
        ;;

    *)
        echo "Error: Unknown app '$APP'"
        echo "Supported apps: agent, config, collector"
        exit 1
        ;;
esac
