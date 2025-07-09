#!/usr/bin/env ucode

import { cursor } from 'uci';
import * as fs from 'fs';

function boolToUci(value) {
    return type(value) == 'bool' ? (value ? '1' : '0') : "" + value;
}

function applySection(ctx, packageName, sectionType, section) {
    let name = section.meta_section;
    if (!name) {
        printf("Error: Section missing 'meta_section'\n");
        return false;
    }

    printf("Configuring %s section: %s in package %s\n", sectionType, name, packageName);

    for (let k in section) {
        if (k == 'meta_section' || k == 'meta_type' || k == 'meta_config') continue;

        let v = section[k];
        let uciValue = boolToUci(v);

        try {
            ctx.set(packageName, name, k, uciValue);
        } catch (e) {
            printf("Error setting %s.%s.%s=%s: %s\n", packageName, name, k, uciValue, "" + e);
            return false;
        }
    }

    printf("Successfully configured section: %s in %s\n", name, packageName);
    return true;
}

function applyWirelessConfig(ctx, config) {
    let wirelessArray = config.device_config?.wireless;
    if (!wirelessArray || type(wirelessArray) != 'array') {
        printf("No wireless configuration found\n");
        return false;
    }

    for (let section in wirelessArray) {
        let pkg = "wireless";
        let t = section.meta_type;
        if (t == 'wifi-device' || t == 'wifi-iface') {
            if (!applySection(ctx, pkg, t, section)) {
                return false;
            }
        }
    }

    return true;
}

function applyWayruConfig(ctx, config) {
    let wayruArray = config.device_config?.wayru;
    if (!wayruArray || type(wayruArray) != 'array') {
        printf("No Wayru configuration found\n");
        return false;
    }

    for (let section in wayruArray) {
        let pkg = section.meta_config;
        let t = section.meta_type;
        if (!applySection(ctx, pkg, t, section)) {
            return false;
        }
    }

    return true;
}

function readConfig(filename) {
    try {
        let file = fs.open(filename, 'r');
        let content = file.read('all');
        file.close();
        return json(content);
    } catch (e) {
        printf("Error reading or parsing config file '%s': %s\n", filename, "" + e);
        return null;
    }
}

function main() {
    let configFile = ARGV[0];
    if (!configFile) {
        printf("Error: No config file specified\n");
        printf("Usage: ucode script.uc <config_file>\n");
        return 1;
    }

    let config = readConfig(configFile);
    if (!config) return 1;

    let ctx;
    try {
        ctx = cursor();
    } catch (e) {
        printf("Error: Failed to initialize UCI cursor: %s\n", "" + e);
        return 1;
    }

    if (!applyWirelessConfig(ctx, config)) {
        printf("Error: Failed to apply wireless configuration\n");
        return 1;
    }

    if (!applyWayruConfig(ctx, config)) {
        printf("Error: Failed to apply Wayru configuration\n");
        return 1;
    }

    // Commit packages
    let packages = ['wireless', 'wayru-agent', 'wayru-collector', 'wayru-config'];
    for (let pkg in packages) {
        try {
            ctx.save(pkg);
            ctx.commit(pkg);
        } catch (e) {
            printf("Error committing %s: %s\n", pkg, "" + e);
            return 1;
        }
    }

    printf("All configuration applied and committed successfully\n");

    printf("Services will be restarted by wayru-config main process\n");
    printf("Configuration applied successfully\n");
    return 0;
}

exit(main());