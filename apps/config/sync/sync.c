#include "sync.h"
#include "core/console.h"
#include "http/http-requests.h"
#include "config.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "renderer/renderer.h"
#include <sys/stat.h>
#include <time.h>
#include <stdint.h> 
#include "token_manager.h"

static Console csl = {.topic = "config-sync"};

#define DEV_GLOBAL_HASH_FILE "./scripts/dev/hashes/global_config.hash"
#define PROD_GLOBAL_HASH_FILE "/etc/wayru-config/hashes/global_config.hash"

// Structure to track application results with error capture
typedef struct {
    bool script_success;
    bool services_restarted_successfully;
    char affected_services[256];      
    char successful_services[256];    // Services that restarted successfully
    char failed_services[256];       // Services that failed to restart
    char error_message[512];          // Detailed error messages
    char service_errors[1024];        // Detailed service restart errors
    char config_hash[16];             
} ConfigApplicationResult;

// Structure to track which services need restart
typedef struct {
    bool wireless;
    bool wayru_agent;
    bool wayru_collector;
    bool wayru_config;
    bool opennds;
} ServiceRestartNeeds;

static uint32_t calculate_djb2_hash(const char *str, size_t length) {
    if (!str) return 0;
    
    uint32_t hash = 5381;
    const unsigned char *ustr = (const unsigned char *)str;
    
    for (size_t i = 0; i < length; i++) {
        hash = ((hash << 5) + hash) + ustr[i];  
    }
    
    return hash;
}

// Helper function for adding services to lists
static void add_service_to_list(char *list, const char *service, bool *first) {
    if (!*first) strcat(list, ", ");
    strcat(list, service);
    *first = false;
}

// Get global hash file path based on mode
static const char *get_global_hash_file_path(bool dev_mode) {
    return dev_mode ? DEV_GLOBAL_HASH_FILE : PROD_GLOBAL_HASH_FILE;
}

// Load global configuration hash from disk
static char *load_global_config_hash(bool dev_mode) {
    const char *hash_file = get_global_hash_file_path(dev_mode);
    
    FILE *fp = fopen(hash_file, "r");
    if (!fp) {
        console_debug(&csl, "No previous global hash file found at %s", hash_file);
        return strdup("0"); // Return "0" instead of empty string
    }
    
    char hash_str[32] = ""; 
    if (fgets(hash_str, sizeof(hash_str), fp)) {
        // Remove newline if present
        char *newline = strchr(hash_str, '\n');
        if (newline) *newline = '\0';
        
        console_debug(&csl, "Loaded global hash: %s", hash_str);
    } else {
        console_warn(&csl, "Failed to read global hash from %s", hash_file);
        strcpy(hash_str, "0");
    }
    
    fclose(fp);
    return strdup(hash_str);
}

// Save global configuration hash to disk
static void save_global_config_hash(bool dev_mode, const char *json_config) {
    if (!json_config) return;
    
    // Calculate hash of RAW configuration 
    size_t json_length = strlen(json_config);
    uint32_t config_hash = calculate_djb2_hash(json_config, json_length);

    const char *hash_file = get_global_hash_file_path(dev_mode);
    FILE *fp = fopen(hash_file, "w");
    if (fp) {
        fprintf(fp, "%u\n", config_hash);
        fclose(fp);
        
        console_debug(&csl, "Saved global config hash %u to %s", config_hash, hash_file);
    } else {
        console_warn(&csl, "Failed to save global hash to %s", hash_file);
    }
}

// Initialize application result with proper defaults
static ConfigApplicationResult init_application_result(void) {
    ConfigApplicationResult result = {
        .script_success = false,
        .services_restarted_successfully = false,
    };
    memset(result.affected_services, 0, sizeof(result.affected_services));
    memset(result.successful_services, 0, sizeof(result.successful_services));
    memset(result.failed_services, 0, sizeof(result.failed_services));
    memset(result.error_message, 0, sizeof(result.error_message));
    memset(result.service_errors, 0, sizeof(result.service_errors));
    memset(result.config_hash, 0, sizeof(result.config_hash));  
    return result;
}

// Helper function to build affected services list
static void build_affected_services_list(const ServiceRestartNeeds *needs, char *output, size_t output_size) {
    output[0] = '\0';  // Start with empty string
    bool first = true;
    
    if (needs->wireless) {
        if (!first) strcat(output, ", ");
        strcat(output, "wireless");
        first = false;
    }
    if (needs->wayru_agent) {
        if (!first) strcat(output, ", ");
        strcat(output, "wayru-agent");
        first = false;
    }
    if (needs->wayru_collector) {
        if (!first) strcat(output, ", ");
        strcat(output, "wayru-collector");
        first = false;
    }
    if (needs->wayru_config) {
        if (!first) strcat(output, ", ");
        strcat(output, "wayru-config");
        first = false;
    }
    if (needs->opennds) {
        if (!first) strcat(output, ", ");
        strcat(output, "opennds");
        first = false;
    }
}

// Generate HTTP result report for backend 
static char* generate_result_report(const ConfigApplicationResult *result) {
    json_object *root = json_object_new_object();
    
    // Determine result status
    const char *result_status;
    if (!result->script_success) {
        result_status = "script_failed";
    } else if (!result->services_restarted_successfully) {
        result_status = "services_failed"; 
    } else {
        result_status = "ok";
    }
    
    json_object *result_obj = json_object_new_string(result_status);
    json_object *affected_obj = json_object_new_string(result->affected_services);
    json_object *successful_obj = json_object_new_string(result->successful_services);
    json_object *failed_obj = json_object_new_string(result->failed_services);

    // Combine script and service errors
    char combined_error[1536] = "";
    if (strlen(result->error_message) > 0) {
        strcat(combined_error, result->error_message);
    }
    if (strlen(result->service_errors) > 0) {
        if (strlen(combined_error) > 0) {
            strcat(combined_error, "; ");
        }
        strcat(combined_error, result->service_errors);
    }
    
    json_object *error_obj = json_object_new_string(combined_error);
    json_object *config_hash_obj = json_object_new_string(result->config_hash);
    
    json_object_object_add(root, "result", result_obj);
    json_object_object_add(root, "affected", affected_obj);
    json_object_object_add(root, "successful", successful_obj);
    json_object_object_add(root, "failed", failed_obj);
    json_object_object_add(root, "error", error_obj);
    json_object_object_add(root, "config_hash", config_hash_obj);
    
    const char *json_string = json_object_to_json_string(root);
    char *result_str = strdup(json_string);
    
    json_object_put(root);
    return result_str;
}

// Send result report to backend 
static void send_result_report_to_backend(const char *result_json, ConfigSyncContext *context) {
    console_info(&csl, "Config application result: %s", result_json);
    
    const char *access_token = sync_get_current_token(context);
    if (!access_token) {
        console_warn(&csl, "No access token available for result report");
        return;
    }
    
    char result_endpoint[512];
    snprintf(result_endpoint, sizeof(result_endpoint), "%s/sync_result", context->endpoint);
    
    HttpPostOptions options = {
        .url = result_endpoint,
        .bearer_token = access_token,
        .body_json_str = result_json,
    };
    
    console_debug(&csl, "Sending config result to: %s", result_endpoint);
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    HttpResult result = http_post(&options);
    
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double duration_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                        (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    
    if (result.is_error) {
        console_warn(&csl, "Failed to send config result to backend: %s - took %.2f ms", 
                    result.error, duration_ms);
    } else if (result.http_status_code >= 200 && result.http_status_code < 300) {
        console_info(&csl, "Config result sent to backend successfully (code: %ld) - took %.2f ms", 
                    result.http_status_code, duration_ms);
    } else {
        console_warn(&csl, "Backend returned error code: %ld - took %.2f ms", 
                    result.http_status_code, duration_ms);
    }
    
    if (result.response_buffer) {
        free(result.response_buffer);
    }    
}

// Analyze which services need restart based on configuration changes
// Uses granular hash comparison per service section
static ServiceRestartNeeds analyze_restart_needs(const char *json, bool dev_mode) {
    ServiceRestartNeeds needs = {false, false, false, false, false};

    // Check each service section for changes using hash comparison
    needs.wireless = config_affects_wireless(json, dev_mode);
    needs.wayru_agent = config_affects_wayru_agent(json, dev_mode);
    needs.wayru_collector = config_affects_wayru_collector(json, dev_mode);
    needs.wayru_config = config_affects_wayru_config(json, dev_mode);
    needs.opennds = config_affects_opennds(json, dev_mode);

    console_debug(&csl, "Restart analysis - wireless: %s, agent: %s, collector: %s, config: %s, opennds: %s",
                 needs.wireless ? "YES" : "no",
                 needs.wayru_agent ? "YES" : "no",
                 needs.wayru_collector ? "YES" : "no",
                 needs.wayru_config ? "YES" : "no",
                 needs.opennds ? "YES" : "no");

    return needs;
}

// Development mode restart simulation with result tracking
static void handle_dev_mode_restart(const ServiceRestartNeeds *needs, ConfigApplicationResult *result) {
    console_info(&csl, "Development mode: simulating service restarts");

    char successful_services[256] = "";
    bool first = true;

    if (needs->wireless) {
        console_info(&csl, "Would reload: wifi configuration");
        add_service_to_list(successful_services, "wireless", &first);
    }
    if (needs->wayru_collector) {
        console_info(&csl, "Would restart: wayru-collector service");
        add_service_to_list(successful_services, "wayru-collector", &first);
    }
    if (needs->wayru_agent) {
        console_info(&csl, "Would restart: wayru-agent service");
        add_service_to_list(successful_services, "wayru-agent", &first);
    }
    if (needs->wayru_config) {
        console_info(&csl, "Would reload: wayru-config configuration");
        add_service_to_list(successful_services, "wayru-config", &first);
    }
    if (needs->opennds) {
        console_info(&csl, "Would restart: opennds service");
        add_service_to_list(successful_services, "opennds", &first);
    }

    if (!needs->wireless && !needs->wayru_collector && !needs->wayru_agent && !needs->wayru_config && !needs->opennds) {
        console_info(&csl, "No services need restart");
    }
    
    // Store successful services (all succeed in dev mode)
    strncpy(result->successful_services, successful_services, sizeof(result->successful_services) - 1);
    result->successful_services[sizeof(result->successful_services) - 1] = '\0';
    
    result->failed_services[0] = '\0'; // No failures in dev mode
    result->services_restarted_successfully = true;
}

// Execute command with error capture
static int execute_service_command(const char *command, const char *service_name, char *error_buffer, size_t error_buffer_size) {
    console_debug(&csl, "Executing: %s", command);
    
    // Execute command and capture both stdout and stderr
    char full_command[512];
    snprintf(full_command, sizeof(full_command), "%s 2>&1", command);
    
    FILE *fp = popen(full_command, "r");
    if (!fp) {
        snprintf(error_buffer, error_buffer_size, "%s: failed to execute command", service_name);
        return -1;
    }
    
    // Read output
    char output[1024] = "";
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(output) + strlen(line) < sizeof(output) - 1) {
            strcat(output, line);
        }
    }
    
    int exit_code = pclose(fp);
    int script_exit_code = WEXITSTATUS(exit_code);
    
    if (script_exit_code != 0) {
        // Clean up output (remove newlines for error message)
        char *newline = strchr(output, '\n');
        if (newline) *newline = '\0';
        
        snprintf(error_buffer, error_buffer_size, "%s: exit code %d%s%s", 
                service_name, script_exit_code,
                strlen(output) > 0 ? " - " : "",
                strlen(output) > 0 ? output : "");
    }
    
    return script_exit_code;
}

// Production mode with detailed error capture
static int restart_services_production(const ServiceRestartNeeds *needs, ConfigApplicationResult *result) {
    console_info(&csl, "Applying configuration changes to services...");
    int total_errors = 0;
    char service_error[256];
    char detailed_errors[1024] = "";
    
    // Arrays to track service results
    char successful_services[256] = "";
    char failed_services[256] = "";
    bool first_success = true;
    bool first_failure = true;

    // 1. Reload WiFi first
    if (needs->wireless) {
        console_info(&csl, "Reloading WiFi configuration...");
        
        if (execute_service_command("wifi reload", "WiFi", service_error, sizeof(service_error)) == 0) {
            console_info(&csl, "WiFi configuration reloaded successfully");
            add_service_to_list(successful_services, "wireless", &first_success);
        } else {
            console_warn(&csl, "WiFi reload failed: %s", service_error);
            total_errors++;
            add_service_to_list(failed_services, "wireless", &first_failure);
            
            if (strlen(detailed_errors) > 0) strcat(detailed_errors, "; ");
            strcat(detailed_errors, service_error);
        }
        sleep(1);
    }

    // 2. Restart OpenNDS
    if (needs->opennds) {
        console_info(&csl, "Restarting OpenNDS...");
        
        if (execute_service_command("/etc/init.d/opennds restart", "OpenNDS", service_error, sizeof(service_error)) == 0) {
            console_info(&csl, "OpenNDS restarted successfully");
            add_service_to_list(successful_services, "opennds", &first_success);
        } else {
            console_warn(&csl, "OpenNDS restart failed: %s", service_error);
            total_errors++;
            add_service_to_list(failed_services, "opennds", &first_failure);
            
            if (strlen(detailed_errors) > 0) strcat(detailed_errors, "; ");
            strcat(detailed_errors, service_error);
        }
        sleep(2);
    }

    // 3. Restart wayru-collector
    if (needs->wayru_collector) {
        console_info(&csl, "Restarting wayru-collector...");
        
        if (execute_service_command("/etc/init.d/wayru-collector reload", "wayru-collector", service_error, sizeof(service_error)) == 0) {
            console_info(&csl, "wayru-collector reloaded successfully");
            add_service_to_list(successful_services, "wayru-collector", &first_success);
        } else {
            console_warn(&csl, "wayru-collector reload failed, trying restart...");
            
            if (execute_service_command("/etc/init.d/wayru-collector restart", "wayru-collector", service_error, sizeof(service_error)) == 0) {
                console_info(&csl, "wayru-collector restarted successfully");
                add_service_to_list(successful_services, "wayru-collector", &first_success);
            } else {
                console_error(&csl, "wayru-collector restart failed: %s", service_error);
                total_errors++;
                add_service_to_list(failed_services, "wayru-collector", &first_failure);
                
                if (strlen(detailed_errors) > 0) strcat(detailed_errors, "; ");
                strcat(detailed_errors, service_error);
            }
        }
        sleep(2);
    }

    // 4. Restart wayru-agent
    if (needs->wayru_agent) {
        console_info(&csl, "Restarting wayru-agent...");
        
        if (execute_service_command("/etc/init.d/wayru-agent reload", "wayru-agent", service_error, sizeof(service_error)) == 0) {
            console_info(&csl, "wayru-agent reloaded successfully");
            add_service_to_list(successful_services, "wayru-agent", &first_success);
        } else {
            console_warn(&csl, "wayru-agent reload failed, trying restart...");
            
            if (execute_service_command("/etc/init.d/wayru-agent restart", "wayru-agent", service_error, sizeof(service_error)) == 0) {
                console_info(&csl, "wayru-agent restarted successfully");
                add_service_to_list(successful_services, "wayru-agent", &first_success);
            } else {
                console_error(&csl, "wayru-agent restart failed: %s", service_error);
                total_errors++;
                add_service_to_list(failed_services, "wayru-agent", &first_failure);
                
                if (strlen(detailed_errors) > 0) strcat(detailed_errors, "; ");
                strcat(detailed_errors, service_error);
            }
        }
        sleep(2);
    }

    // 5. Reload wayru-config last
    if (needs->wayru_config) {
        console_info(&csl, "wayru-config configuration changed, triggering reload...");
        
        if (execute_service_command("/etc/init.d/wayru-config reload", "wayru-config", service_error, sizeof(service_error)) == 0) {
            console_info(&csl, "wayru-config reload triggered successfully");
            add_service_to_list(successful_services, "wayru-config", &first_success);
        } else {
            console_warn(&csl, "wayru-config reload failed: %s", service_error);
            total_errors++;
            add_service_to_list(failed_services, "wayru-config", &first_failure);
            
            if (strlen(detailed_errors) > 0) strcat(detailed_errors, "; ");
            strcat(detailed_errors, service_error);
        }
    }

    // Store successful and failed services separately
    strncpy(result->successful_services, successful_services, sizeof(result->successful_services) - 1);
    result->successful_services[sizeof(result->successful_services) - 1] = '\0';
    
    strncpy(result->failed_services, failed_services, sizeof(result->failed_services) - 1);
    result->failed_services[sizeof(result->failed_services) - 1] = '\0';

    // Set detailed results
    if (total_errors == 0) {
        console_info(&csl, "All service operations completed successfully");
        result->services_restarted_successfully = true;
    } else {
        console_warn(&csl, "Service restart completed with %d errors", total_errors);
        result->services_restarted_successfully = false;
        
        // Store detailed service errors
        strncpy(result->service_errors, detailed_errors, sizeof(result->service_errors) - 1);
        result->service_errors[sizeof(result->service_errors) - 1] = '\0';
    }

    return total_errors;
}

//Fetch device configuration from remote endpoint with global hash sync
// Returns allocated JSON string or NULL on failure
char *fetch_device_config_json(const char *endpoint, ConfigSyncContext *context) {
    if (!endpoint || strlen(endpoint) == 0) {
        console_error(&csl, "Missing config endpoint");
        return NULL;
    }

    const char *access_token = sync_get_current_token(context);
    if (!access_token) {
        console_warn(&csl, "No valid access token available, aborting config request");
        return NULL;
    }

    // Load current global config hash and send to backend
    char *current_hash = load_global_config_hash(context->dev_mode);
    console_debug(&csl, "Current global config hash: '%s'", current_hash ? current_hash : "null");

    char sync_endpoint[512];
    snprintf(sync_endpoint, sizeof(sync_endpoint), "%s/sync", endpoint);

    console_debug(&csl, "Making config sync request to: %s", sync_endpoint);

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Create JSON body with current config hash
    json_object *sync_body = json_object_new_object();
    json_object *hash_obj = json_object_new_string(current_hash ? current_hash : "");
    json_object_object_add(sync_body, "current_config_hash", hash_obj);
    
    const char *sync_json = json_object_to_json_string(sync_body);

    HttpPostOptions options = {
        .url = sync_endpoint,
        .bearer_token = access_token,
        .body_json_str = sync_json,
    };

    HttpResult result = http_post(&options);

    // Calculate request duration
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double duration_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    // Cleanup sync body
    json_object_put(sync_body);
    if (current_hash) free(current_hash);

    if (result.http_status_code == 200) {
        // HTTP 200 - Configuration update available
        if (result.response_buffer) {
            console_info(&csl, "Configuration update available (HTTP 200) - took %.2f ms", duration_ms);
            
            size_t json_length = strlen(result.response_buffer);
            console_info(&csl, "Received updated config JSON (%zu bytes): %.200s%s",
                         json_length,
                         result.response_buffer,
                         json_length > 200 ? "..." : "");

            return result.response_buffer;
        } else {
            console_warn(&csl, "HTTP 200 but no response body - took %.2f ms", duration_ms);
            return NULL;
        }

    } else if (result.http_status_code == 304) {
        // HTTP 304
        console_info(&csl, "Configuration unchanged (HTTP 304 Not Modified) - took %.2f ms", duration_ms);
        console_debug(&csl, "Server confirmed no configuration changes since last sync");
    
        if (result.response_buffer) {
            free(result.response_buffer);
        }
        return NULL;  // No config to process
    
    } else if (result.is_error || !result.response_buffer) {
        // General error handling AFTER specific status checks
        console_warn(&csl, "Config sync request failed: %s - took %.2f ms",
                    result.error ? result.error : "unknown error", duration_ms);
        
        if (result.response_buffer) {
            free(result.response_buffer);
        }
        return NULL;
        
    } else {
        // Other HTTP status 
        console_warn(&csl, "Config sync request failed with code: %ld - took %.2f ms",
                    result.http_status_code, duration_ms);
        
        if (result.response_buffer) {
            free(result.response_buffer);
        }
        return NULL;
    }
}

static void config_sync_task(void *ctx) {
    ConfigSyncContext *context = (ConfigSyncContext *)ctx;

    console_debug(&csl, "Executing config sync task");

    if (!sync_is_token_valid(context)) {
        console_info(&csl, "Access token expired, attempting refresh...");
        int ret = sync_refresh_access_token(context);
        if (ret < 0) {
            console_warn(&csl, "Failed to refresh token, skipping this cycle");
            return;
        }
    }

    char *json = fetch_device_config_json(context->endpoint, context);
    if (!json) {
        console_debug(&csl, "No configuration update needed or failed to fetch, skipping this cycle");
        return;
    }

    console_info(&csl, "Configuration update received, analyzing changes...");

    // Calculate global hash for feedback 
    size_t json_length = strlen(json);
    uint32_t global_hash = calculate_djb2_hash(json, json_length);

    // Analyze what services need restart using granular hash comparison
    ServiceRestartNeeds needs = analyze_restart_needs(json, context->dev_mode);

    // Skip if no changes detected at granular level
    if (!needs.wireless && !needs.wayru_agent && !needs.wayru_collector && !needs.wayru_config && !needs.opennds) {
        console_info(&csl, "No granular configuration changes detected, skipping application");

        // Save global hash 
        save_global_config_hash(context->dev_mode, json);
        free(json);
        return;
    }

    console_info(&csl, "Granular configuration changes detected, applying updates...");

    // Initialize result tracking with affected services and hash
    ConfigApplicationResult app_result = init_application_result();
    build_affected_services_list(&needs, app_result.affected_services, sizeof(app_result.affected_services));

    // Store global hash in result for feedback
    snprintf(app_result.config_hash, sizeof(app_result.config_hash), "%u", global_hash);
    
    // Apply configuration to UCI with result tracking
    if (apply_config_without_restarts(json, context->dev_mode) == 0) {
        console_info(&csl, "Configuration applied successfully");
        app_result.script_success = true;

        // Save global configuration hash after successful application
        save_global_config_hash(context->dev_mode, json);

        // Save granular hashes (handled automatically by config_affects_* functions)
        console_debug(&csl, "Granular service hashes updated during analysis phase");

        // CRITICAL: ONLY RESTART SERVICES IF SCRIPT WAS SUCCESSFUL
        if (context->dev_mode) {
            handle_dev_mode_restart(&needs, &app_result);
        } else {
            restart_services_production(&needs, &app_result);
        }

    } else {
        console_error(&csl, "Failed to apply configuration - skipping service restarts");
        app_result.script_success = false;
        strncpy(app_result.error_message, "Configuration script failed", sizeof(app_result.error_message) - 1);
        // Hash is still included even if script failed (for debugging)
    }

    // Generate and send result report to backend
    char *result_report = generate_result_report(&app_result);
    if (result_report) {
        send_result_report_to_backend(result_report, context);
        free(result_report);
    }

    free(json);
}

// Initialize and start the configuration sync service
// Sets up periodic task and renderer configuration
ConfigSyncContext *start_config_sync_service(const char *endpoint,
                                            uint32_t initial_delay_ms,
                                            uint32_t interval_ms,
                                            bool dev_mode) {
    // Allocate context structure
    ConfigSyncContext *context = (ConfigSyncContext *)malloc(sizeof(ConfigSyncContext));
    if (!context) {
        console_error(&csl, "Failed to allocate memory for config sync context");
        return NULL;
    }

    // Initialize context
    context->endpoint = endpoint;
    context->dev_mode = dev_mode;
    context->current_interval_ms = interval_ms;

    // Initialize token management
    memset(context->access_token, 0, sizeof(context->access_token));
    context->token_expiry = 0;
    context->token_initialized = false;

    // Configure renderer for appropriate mode
    set_renderer_dev_mode(dev_mode);

    console_info(&csl, "Section hashes will be stored in: %s",
                 dev_mode ? "./scripts/dev/hashes" : "/etc/wayru-config/hashes");
    
    console_info(&csl, "Global config hash will be stored in: %s",
                 dev_mode ? DEV_GLOBAL_HASH_FILE : PROD_GLOBAL_HASH_FILE);

    console_info(&csl, "Starting config sync service with initial delay %u ms, interval %u ms",
                 initial_delay_ms, interval_ms);

    // Try to get initial token
    console_info(&csl, "Attempting to acquire initial access token...");
    int token_ret = sync_refresh_access_token(context);
    if (token_ret == 0) {
        console_info(&csl, "Initial access token acquired successfully");
    } else {
        console_warn(&csl, "Failed to acquire initial token, will retry during operation");
    }

    // Schedule the periodic task
    context->task_id = schedule_repeating(initial_delay_ms, interval_ms, config_sync_task, context);
    if (context->task_id == 0) {
        console_error(&csl, "Failed to schedule config sync task");
        free(context);
        return NULL;
    }

    console_debug(&csl, "Successfully scheduled config sync task with ID %u", context->task_id);
    return context;
}

// Clean up configuration sync context and cancel scheduled task
void clean_config_sync_context(ConfigSyncContext *context) {
    if (!context) {
        return;
    }

    console_info(&csl, "Cleaning config sync context...");

    // Cancel scheduled task
    if (context->task_id != 0) {
        console_debug(&csl, "Cancelling sync task ID: %u", context->task_id);
        cancel_task(context->task_id);
        context->task_id = 0;
    }

    // Clear token from memory
    if (context->token_initialized) {
        console_debug(&csl, "Clearing access token from memory");
        memset(context->access_token, 0, sizeof(context->access_token));
        context->token_initialized = false;
        context->token_expiry = 0;
    }

    // Free context memory
    console_debug(&csl, "Freeing sync context memory");
    free(context);

    console_info(&csl, "Config sync context cleaned successfully");
}
