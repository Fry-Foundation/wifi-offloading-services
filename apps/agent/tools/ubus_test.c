#include "../services/ubus_client.h"
#include "core/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

static Console csl = {
    .topic = "ubus_test",
};

// Print usage information
static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <service> <method> [args]\n", program_name);
    printf("\nOPTIONS:\n");
    printf("  -t, --timeout TIMEOUT    Set timeout in milliseconds (default: 5000)\n");
    printf("  -j, --json ARGS          Pass JSON arguments to method\n");
    printf("  -l, --list               List all available services\n");
    printf("  -m, --methods SERVICE    List methods for a specific service\n");
    printf("  -p, --ping SERVICE       Ping a specific service\n");
    printf("  -a, --agent              Test wayru-agent service methods\n");
    printf("  -v, --verbose            Enable verbose output\n");
    printf("  -h, --help               Show this help message\n");
    printf("\nEXAMPLES:\n");
    printf("  %s wayru-agent ping\n", program_name);
    printf("  %s wayru-agent get_status\n", program_name);
    printf("  %s wayru-agent get_device_info\n", program_name);
    printf("  %s wayru-agent get_access_token\n", program_name);
    printf("  %s -l\n", program_name);
    printf("  %s -m wayru-agent\n", program_name);
    printf("  %s -p wayru-agent\n", program_name);
    printf("  %s -a\n", program_name);
}

// Print response in a formatted way
static void print_response(UbusResponse *response, bool verbose) {
    if (!response) {
        printf("No response received\n");
        return;
    }

    if (response->success) {
        printf("SUCCESS:\n");
        if (response->json_response) {
            printf("%s\n", response->json_response);
        } else {
            printf("(no data)\n");
        }
    } else {
        printf("ERROR: %s (code: %d)\n", 
               response->error_message ? response->error_message : "Unknown error",
               response->error_code);
    }

    if (verbose) {
        printf("\nVerbose Info:\n");
        printf("  Success: %s\n", response->success ? "true" : "false");
        printf("  Error Code: %d\n", response->error_code);
        printf("  Error Message: %s\n", response->error_message ? response->error_message : "none");
        printf("  Has Data: %s\n", response->data ? "true" : "false");
        printf("  JSON Response: %s\n", response->json_response ? "available" : "none");
    }
}

// Test all wayru-agent methods
static void test_agent_methods(UbusClient *client, bool verbose) {
    printf("Testing wayru-agent service methods...\n\n");

    const char *methods[] = {
        "ping",
        "get_status", 
        "get_device_info",
        "get_access_token",
        "get_registration"
    };

    int num_methods = sizeof(methods) / sizeof(methods[0]);

    for (int i = 0; i < num_methods; i++) {
        printf("=== Testing method: %s ===\n", methods[i]);
        
        UbusResponse *response = ubus_client_call(client, "wayru-agent", methods[i], NULL);
        print_response(response, verbose);
        
        if (response) {
            ubus_response_free(response);
        }
        
        printf("\n");
    }
}

// List all services
static void list_services(UbusClient *client, bool verbose) {
    printf("Listing all UBUS services...\n\n");
    
    UbusResponse *response = ubus_client_list_services(client);
    print_response(response, verbose);
    
    if (response) {
        ubus_response_free(response);
    }
}

// List methods for a service
static void list_service_methods(UbusClient *client, const char *service_name, bool verbose) {
    printf("Listing methods for service: %s\n\n", service_name);
    
    UbusResponse *response = ubus_client_get_service_methods(client, service_name);
    print_response(response, verbose);
    
    if (response) {
        ubus_response_free(response);
    }
}

// Ping a service
static void ping_service(UbusClient *client, const char *service_name, bool verbose) {
    printf("Pinging service: %s\n\n", service_name);
    
    bool result = ubus_client_ping_service(client, service_name);
    printf("Ping result: %s\n", result ? "SUCCESS" : "FAILED");
    
    if (verbose) {
        printf("Service '%s' is %s\n", service_name, result ? "available" : "not available");
    }
}

int main(int argc, char *argv[]) {
    int timeout = 0;
    char *json_args = NULL;
    bool list_flag = false;
    bool methods_flag = false;
    bool ping_flag = false;
    bool agent_test = false;
    bool verbose = false;
    char *service_name = NULL;
    char *method_name = NULL;

    static struct option long_options[] = {
        {"timeout", required_argument, 0, 't'},
        {"json", required_argument, 0, 'j'},
        {"list", no_argument, 0, 'l'},
        {"methods", required_argument, 0, 'm'},
        {"ping", required_argument, 0, 'p'},
        {"agent", no_argument, 0, 'a'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "t:j:lm:p:avh", long_options, &option_index)) != -1) {
        switch (c) {
            case 't':
                timeout = atoi(optarg);
                break;
            case 'j':
                json_args = optarg;
                break;
            case 'l':
                list_flag = true;
                break;
            case 'm':
                methods_flag = true;
                service_name = optarg;
                break;
            case 'p':
                ping_flag = true;
                service_name = optarg;
                break;
            case 'a':
                agent_test = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case '?':
                print_usage(argv[0]);
                return 1;
            default:
                abort();
        }
    }

    // Initialize UBUS client
    console_info(&csl, "Initializing UBUS client...");
    UbusClient *client = ubus_client_init(timeout);
    if (!client) {
        console_error(&csl, "Failed to initialize UBUS client");
        return 1;
    }

    // Check if client is connected
    if (!ubus_client_is_connected(client)) {
        console_error(&csl, "UBUS client is not connected");
        ubus_client_cleanup(client);
        return 1;
    }

    console_info(&csl, "UBUS client connected successfully");

    // Handle different operations
    if (list_flag) {
        list_services(client, verbose);
    } else if (methods_flag) {
        list_service_methods(client, service_name, verbose);
    } else if (ping_flag) {
        ping_service(client, service_name, verbose);
    } else if (agent_test) {
        test_agent_methods(client, verbose);
    } else if (optind < argc) {
        // Regular method call
        service_name = argv[optind];
        if (optind + 1 < argc) {
            method_name = argv[optind + 1];
        } else {
            console_error(&csl, "Method name required");
            print_usage(argv[0]);
            ubus_client_cleanup(client);
            return 1;
        }

        printf("Calling %s.%s...\n\n", service_name, method_name);

        UbusResponse *response;
        if (json_args) {
            response = ubus_client_call_json(client, service_name, method_name, json_args);
        } else {
            response = ubus_client_call(client, service_name, method_name, NULL);
        }

        print_response(response, verbose);

        if (response) {
            ubus_response_free(response);
        }
    } else {
        console_error(&csl, "No operation specified");
        print_usage(argv[0]);
        ubus_client_cleanup(client);
        return 1;
    }

    // Cleanup
    ubus_client_cleanup(client);
    console_info(&csl, "UBUS client test completed");

    return 0;
}