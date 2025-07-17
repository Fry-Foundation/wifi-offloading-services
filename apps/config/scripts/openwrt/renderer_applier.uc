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
        
        // HANDLE ARRAY VALUES AS UCI LISTS
        if (type(v) == 'array') {
            printf("Setting list %s.%s.%s with %d items\n", packageName, name, k, length(v));
            
            try {
                // Clear existing list first by setting to empty array
                ctx.set(packageName, name, k, []);
            } catch (e) {
                // If section doesn't exist, create it first
                try {
                    ctx.add(packageName, sectionType, name);
                    ctx.set(packageName, name, k, []);
                } catch (e2) {
                    printf("Error creating section %s.%s: %s\n", packageName, name, "" + e2);
                    return false;
                }
            }
            
            // Set the entire array at once
            try {
                ctx.set(packageName, name, k, v);
                printf("Successfully set list %s.%s.%s with %d items\n", packageName, name, k, length(v));
            } catch (e) {
                printf("Error setting list %s.%s.%s: %s\n", packageName, name, k, "" + e);
                return false;
            }
        } else {
            // Handle single values
            let uciValue = boolToUci(v);
            try {
                ctx.set(packageName, name, k, uciValue);
            } catch (e) {
                // If section doesn't exist, create it first
                try {
                    ctx.add(packageName, sectionType, name);
                    ctx.set(packageName, name, k, uciValue);
                } catch (e2) {
                    printf("Error setting %s.%s.%s=%s: %s\n", packageName, name, k, uciValue, "" + e2);
                    return false;
                }
            }
        }
    }

    printf("Successfully configured section: %s in %s\n", name, packageName);
    return true;
}

function applyWirelessConfig(ctx, config) {
    let wirelessArray = config.device_config?.wireless;
    if (!wirelessArray || type(wirelessArray) != 'array') {
        printf("No wireless configuration found\n");
        return true; // ✅ RETURN TRUE - NOT AN ERROR IF NO CONFIG
    }

    printf("Applying wireless configuration...\n");
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
        return true; 
    }

    printf("Applying Wayru services configuration...\n");
    for (let section in wayruArray) {
        let pkg = section.meta_config;
        let t = section.meta_type;
        if (!applySection(ctx, pkg, t, section)) {
            return false;
        }
    }

    return true;
}

function applyOpenNdsConfig(ctx, config) {
    let openndsArray = config.device_config?.opennds;
    if (!openndsArray || type(openndsArray) != 'array') {
        printf("No OpenNDS configuration found\n");
        return true; 
    }

    printf("Applying OpenNDS configuration...\n");
    for (let section in openndsArray) {
        let pkg = "opennds";
        let t = section.meta_type || "opennds";
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

    printf("Starting configuration application...\n");

    if (!applyWirelessConfig(ctx, config)) {
        printf("Error: Failed to apply wireless configuration\n");
        return 1;
    }

    if (!applyWayruConfig(ctx, config)) {
        printf("Error: Failed to apply Wayru configuration\n");
        return 1;
    }

    if (!applyOpenNdsConfig(ctx, config)) {  
        printf("Error: Failed to apply OpenNDS configuration\n");
        return 1;
    }

    // COMMIT PACKAGES WITH ERROR HANDLING
    printf("Committing configuration changes...\n");
    let packages = ['wireless', 'wayru-agent', 'wayru-collector', 'wayru-config', 'opennds'];
    
    for (let pkg in packages) {
        try {
            printf("Saving and committing package: %s\n", pkg);
            ctx.save(pkg);
            ctx.commit(pkg);
            printf("Successfully committed package: %s\n", pkg);
        } catch (e) {
            // WARNING INSTEAD OF ERROR - SOME PACKAGES MAY NOT EXIST
            printf("Warning: Error committing %s: %s\n", pkg, "" + e);
            // Continue with other packages instead of failing completely
        }
    }

    printf("Configuration application completed successfully\n");
    printf("Services will be restarted by wayru-config main process\n");
    return 0;
}

exit(main());