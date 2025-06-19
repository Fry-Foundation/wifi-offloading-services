#!/usr/bin/env ucode

import { cursor } from 'uci';
import * as fs from 'fs';

// Helper function to convert JSON boolean to UCI string
function boolToUci(value) {
    if (type(value) == 'bool') {
        return value ? '1' : '0';
    }
    return string(value);
}

// Apply wireless interface configuration
function applyWirelessInterface(ctx, interfaceConfig) {
    const name = interfaceConfig.name;
    if (!name) {
        printf("Error: Interface missing required 'name' field\n");
        return false;
    }

    printf("Configuring wireless interface: %s\n", name);

    // Set each property from the interface configuration
    for (let key in interfaceConfig) {
        if (key == 'name') continue; // Skip the name field as it's the section identifier

        let uciValue;
        if (type(interfaceConfig[key]) == 'bool') {
            uciValue = boolToUci(interfaceConfig[key]);
        } else {
            uciValue = string(interfaceConfig[key]);
        }

        try {
            ctx.set('wireless', name, key, uciValue);
        } catch (e) {
            printf("Error setting wireless.%s.%s=%s: %s\n",
                   name, key, uciValue, e.message || 'unknown error');
            return false;
        }
    }

    printf("Successfully configured interface: %s\n", name);
    return true;
}

// Apply all wireless interfaces from configuration
function applyWirelessInterfaces(ctx, config) {
    const wireless = config.wireless;
    if (!wireless) {
        printf("No wireless configuration found\n");
        return false;
    }

    const interfaces = wireless.interfaces;
    if (!interfaces || type(interfaces) != 'array') {
        printf("No wireless interfaces found\n");
        return false;
    }

    // Apply each interface configuration
    for (let interfaceConfig in interfaces) {
        if (!applyWirelessInterface(ctx, interfaceConfig)) {
            return false;
        }
    }

    return true;
}

// Read and parse JSON configuration file
function readConfig(filename) {
    let file;
    try {
        file = fs.open(filename, 'r');
    } catch (e) {
        printf("Error: Cannot open config file '%s': %s\n",
               filename, e.message || 'unknown error');
        return null;
    }

    let content;
    try {
        content = file.read('all');
        file.close();
    } catch (e) {
        printf("Error: Cannot read config file '%s': %s\n",
               filename, e.message || 'unknown error');
        file.close();
        return null;
    }

    let config;
    try {
        config = json(content);
    } catch (e) {
        printf("Error: Failed to parse JSON: %s\n", e.message || 'unknown error');
        return null;
    }

    return config;
}

// Main function
function main() {
    const configFile = ARGV[1] || 'config.example.json';

    // Read configuration
    const rootConfig = readConfig(configFile);
    if (!rootConfig) {
        return 1;
    }

    const config = rootConfig.config;
    if (!config) {
        printf("Error: No 'config' object found in JSON\n");
        return 1;
    }

    // Initialize UCI cursor
    let ctx;
    try {
        ctx = cursor();
    } catch (e) {
        printf("Error: Failed to initialize UCI cursor: %s\n",
               e.message || 'unknown error');
        return 1;
    }

    // Apply wireless configuration
    if (!applyWirelessInterfaces(ctx, config)) {
        printf("Error: Failed to apply wireless configuration\n");
        return 1;
    }

    // Save changes
    try {
        ctx.save('wireless');
    } catch (e) {
        printf("Error: Failed to save wireless configuration: %s\n",
               e.message || 'unknown error');
        return 1;
    }

    // Commit changes
    try {
        ctx.commit('wireless');
    } catch (e) {
        printf("Error: Failed to commit wireless configuration: %s\n",
               e.message || 'unknown error');
        return 1;
    }

    printf("Wireless configuration applied and committed successfully\n");
    return 0;
}

// Run main function and exit with appropriate code
exit(main());
