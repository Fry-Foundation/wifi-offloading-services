#!/usr/bin/env ucode

import * as fs from 'fs';

function boolToUci(value) {
    return type(value) == 'bool' ? (value ? '1' : '0') : "" + value;
}

function printUciCommand(packageName, sectionName, key, value) {
    printf("uci set %s.%s.%s='%s'\n", packageName, sectionName, key, value);
}

function renderSection(packageName, sectionType, section) {
    let name = section.meta_section;
    if (!name) {
        printf("Error: Section missing 'meta_section'\n");
        return false;
    }

    printf("\n# Configuring %s section: %s in package %s\n", sectionType, name, packageName);
    
    for (let k in section) {
        if (k == 'meta_section' || k == 'meta_type' || k == 'meta_config') continue;

        let v = section[k];
        let uciValue = boolToUci(v);
        printUciCommand(packageName, name, k, uciValue);
    }

    return true;
}

function renderWirelessConfig(config) {
    let wirelessArray = config.device_config?.wireless;
    if (!wirelessArray || type(wirelessArray) != 'array') {
        printf("# No wireless configuration found\n");
        return false;
    }

    printf("\n# === Wireless Configuration ===\n");
    for (let section in wirelessArray) {
        let pkg = "wireless";
        let t = section.meta_type;
        if (t == 'wifi-device' || t == 'wifi-iface') {
            renderSection(pkg, t, section);
        }
    }
    printf("uci commit wireless\n");
    printf("# WiFi reload will be handled by wayru-config main process\n");
    return true;
}

function renderWayruConfig(config) {
    let wayruArray = config.device_config?.wayru;
    if (!wayruArray || type(wayruArray) != 'array') {
        printf("# No Wayru configuration found\n");
        return false;
    }

    printf("\n# === Wayru Services Configuration ===\n");
    for (let section in wayruArray) {
        let pkg = section.meta_config;
        let t = section.meta_type;
        renderSection(pkg, t, section);
        printf("uci commit %s\n", pkg);
        printf("# %s restart will be handled by wayru-config main process\n", pkg);
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

    renderWirelessConfig(config);
    renderWayruConfig(config);

    printf("\n# === End of UCI Commands ===\n");
    return 0;
}

exit(main());