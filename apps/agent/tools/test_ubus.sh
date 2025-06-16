#!/bin/bash

# UBUS Integration Test Script for Wayru Agent
# This script tests the UBUS server and client functionality

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_UTIL="$SCRIPT_DIR/bin/ubus_test"
AGENT_SERVICE="wayru-agent"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_TESTS++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_TESTS++))
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Test wrapper function
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_success="${3:-true}"

    ((TOTAL_TESTS++))

    log_info "Running test: $test_name"

    if [ "$expected_success" = "true" ]; then
        if eval "$test_command" >/dev/null 2>&1; then
            log_success "$test_name"
        else
            log_error "$test_name"
        fi
    else
        if eval "$test_command" >/dev/null 2>&1; then
            log_error "$test_name (expected to fail but passed)"
        else
            log_success "$test_name (correctly failed as expected)"
        fi
    fi
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check if ubus command exists
    if ! command -v ubus >/dev/null 2>&1; then
        log_error "ubus command not found. Make sure OpenWrt UBUS is installed."
        exit 1
    fi

    # Check if test utility exists
    if [ ! -f "$TEST_UTIL" ]; then
        log_warn "Test utility not found at $TEST_UTIL"
        log_info "Building test utility..."

        cd "$SCRIPT_DIR"
        if make >/dev/null 2>&1; then
            log_success "Test utility built successfully"
        else
            log_error "Failed to build test utility"
            exit 1
        fi
    fi

    # Check if wayru-agent service is running
    if ! pgrep -f "wayru-os-services\|agent" >/dev/null; then
        log_warn "Wayru agent process not found. Some tests may fail."
        log_info "You can start the agent with: /usr/bin/agent"
    fi

    log_success "Prerequisites check completed"
}

# Test UBUS daemon
test_ubus_daemon() {
    log_info "Testing UBUS daemon..."

    run_test "UBUS daemon connectivity" "ubus list"
    run_test "UBUS system service" "ubus list system"
}

# Test wayru-agent service availability
test_agent_service() {
    log_info "Testing wayru-agent service..."

    run_test "wayru-agent service exists" "ubus list | grep -q wayru-agent"
    run_test "wayru-agent methods available" "ubus list wayru-agent"
}

# Test individual methods
test_agent_methods() {
    log_info "Testing wayru-agent methods..."

    # Test ping method
    run_test "ping method" "ubus call wayru-agent ping"

    # Test get_status method
    run_test "get_status method" "ubus call wayru-agent get_status"

    # Test get_device_info method
    run_test "get_device_info method" "ubus call wayru-agent get_device_info"

    # Test get_access_token method
    run_test "get_access_token method" "ubus call wayru-agent get_access_token"

    # Test get_registration method
    run_test "get_registration method" "ubus call wayru-agent get_registration"
}

# Test our custom utility
test_custom_utility() {
    log_info "Testing custom UBUS utility..."

    run_test "utility help" "$TEST_UTIL --help"
    run_test "utility ping wayru-agent" "$TEST_UTIL -p wayru-agent"
    run_test "utility list services" "$TEST_UTIL -l"
    run_test "utility list wayru-agent methods" "$TEST_UTIL -m wayru-agent"
    run_test "utility call ping" "$TEST_UTIL wayru-agent ping"
    run_test "utility call get_status" "$TEST_UTIL wayru-agent get_status"
    run_test "utility test all agent methods" "$TEST_UTIL -a"
}

# Test error conditions
test_error_conditions() {
    log_info "Testing error conditions..."

    run_test "nonexistent service" "$TEST_UTIL nonexistent-service ping" false
    run_test "nonexistent method" "$TEST_UTIL wayru-agent nonexistent-method" false
    run_test "invalid JSON args" "$TEST_UTIL -j 'invalid-json' wayru-agent ping" false
}

# Performance tests
test_performance() {
    log_info "Testing performance..."

    # Test response time
    local start_time=$(date +%s%N)
    if ubus call wayru-agent ping >/dev/null 2>&1; then
        local end_time=$(date +%s%N)
        local duration=$(( (end_time - start_time) / 1000000 )) # Convert to milliseconds

        if [ $duration -lt 1000 ]; then # Less than 1 second
            log_success "ping response time: ${duration}ms (acceptable)"
        else
            log_warn "ping response time: ${duration}ms (slow)"
        fi
    else
        log_error "ping performance test failed"
    fi

    # Test multiple rapid calls
    log_info "Testing rapid consecutive calls..."
    local rapid_test_passed=true
    for i in {1..5}; do
        if ! ubus call wayru-agent ping >/dev/null 2>&1; then
            rapid_test_passed=false
            break
        fi
    done

    if [ "$rapid_test_passed" = "true" ]; then
        log_success "rapid consecutive calls test"
        ((PASSED_TESTS++))
    else
        log_error "rapid consecutive calls test"
        ((FAILED_TESTS++))
    fi
    ((TOTAL_TESTS++))
}

# Detailed service validation
validate_service_responses() {
    log_info "Validating service responses..."

    # Test ping response format
    local ping_response=$(ubus call wayru-agent ping 2>/dev/null || echo "")
    if echo "$ping_response" | grep -q '"response": "pong"'; then
        log_success "ping response format validation"
    else
        log_error "ping response format validation"
    fi
    ((TOTAL_TESTS++))

    # Test status response format
    local status_response=$(ubus call wayru-agent get_status 2>/dev/null || echo "")
    if echo "$status_response" | grep -q '"service": "wayru-agent"'; then
        log_success "status response format validation"
    else
        log_error "status response format validation"
    fi
    ((TOTAL_TESTS++))
}

# Monitor UBUS traffic (if available)
monitor_ubus_traffic() {
    log_info "Monitoring UBUS traffic..."

    # Start monitoring in background for a short time
    timeout 5s ubus monitor 2>/dev/null | grep wayru-agent &
    local monitor_pid=$!

    # Make some calls to generate traffic
    ubus call wayru-agent ping >/dev/null 2>&1 || true
    ubus call wayru-agent get_status >/dev/null 2>&1 || true

    # Wait for monitor to finish
    wait $monitor_pid 2>/dev/null || true

    log_info "UBUS traffic monitoring completed"
}

# Main test execution
main() {
    echo "=================================================="
    echo "  UBUS Integration Test Suite for Wayru Agent"
    echo "=================================================="
    echo

    check_prerequisites
    echo

    test_ubus_daemon
    echo

    test_agent_service
    echo

    test_agent_methods
    echo

    test_custom_utility
    echo

    test_error_conditions
    echo

    test_performance
    echo

    validate_service_responses
    echo

    # Optional: Monitor UBUS traffic (might not work in all environments)
    if command -v timeout >/dev/null 2>&1; then
        monitor_ubus_traffic
        echo
    fi

    # Print test summary
    echo "=================================================="
    echo "                 TEST SUMMARY"
    echo "=================================================="
    echo -e "Total Tests:  ${BLUE}$TOTAL_TESTS${NC}"
    echo -e "Passed Tests: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "Failed Tests: ${RED}$FAILED_TESTS${NC}"
    echo

    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}🎉 All tests passed!${NC}"
        echo
        echo "Your UBUS integration is working correctly."
        echo
        echo "Next steps:"
        echo "1. Use 'ubus call wayru-agent <method>' to interact with the agent"
        echo "2. Use '$TEST_UTIL -a' to test all methods"
        echo "3. Check the logs with 'logread | grep wayru' for detailed information"
        exit 0
    else
        echo -e "${RED}❌ Some tests failed.${NC}"
        echo
        echo "Troubleshooting steps:"
        echo "1. Check if the wayru agent is running: 'pgrep -f agent'"
        echo "2. Check UBUS daemon status: '/etc/init.d/ubus status'"
        echo "3. Check system logs: 'logread | grep -E \"wayru|ubus\"'"
        echo "4. Try restarting the agent: '/etc/init.d/wayru-os-services restart'"
        exit 1
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [OPTIONS]"
        echo
        echo "OPTIONS:"
        echo "  --help, -h     Show this help message"
        echo "  --build, -b    Build test utility only"
        echo "  --quick, -q    Run quick tests only"
        echo
        echo "This script tests the UBUS integration for the Wayru Agent."
        echo "It verifies that the wayru-agent service is accessible via UBUS"
        echo "and that all methods work correctly."
        exit 0
        ;;
    --build|-b)
        log_info "Building test utility..."
        cd "$SCRIPT_DIR"
        make
        log_success "Test utility built at $TEST_UTIL"
        exit 0
        ;;
    --quick|-q)
        log_info "Running quick tests only..."
        check_prerequisites
        test_ubus_daemon
        test_agent_service
        run_test "quick ping test" "ubus call wayru-agent ping"

        echo "Quick test completed: $PASSED_TESTS/$TOTAL_TESTS passed"
        exit 0
        ;;
    "")
        # No arguments, run full test suite
        main
        ;;
    *)
        echo "Unknown option: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac
