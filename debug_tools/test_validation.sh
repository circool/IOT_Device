#!/usr/bin/env bash
# debug_tools/test_validation.sh
# Server-side validation test suite for config form
# Usage: ./test_validation.sh [IP_ADDRESS]

set -euo pipefail

# ============================================================================
# CONFIGURATION
# ============================================================================

DEFAULT_IP="192.168.100.97"
IP="${1:-$DEFAULT_IP}"
BASE_URL="http://$IP"
TIMEOUT=3

# ============================================================================
# COLORS
# ============================================================================

if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[0;33m'
    NC='\033[0m'
else
    GREEN=''
    RED=''
    YELLOW=''
    NC=''
fi

# ============================================================================
# HELPERS
# ============================================================================

log_info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

check_connection() {
    if ! curl -s -o /dev/null --connect-timeout "$TIMEOUT" "$BASE_URL/"; then
        log_error "Device not reachable at $BASE_URL"
        exit 1
    fi
}

extract_value() {
    local field="$1"
    local html="$2"
    echo "$html" | grep -o "name=\"$field\" value=\"[^\"]*\"" | head -1 | sed 's/.*value="\([^"]*\)".*/\1/' || echo ""
}

read_config() {
    local html
    html=$(curl -s --connect-timeout "$TIMEOUT" "$BASE_URL/config")
    
    CURRENT_SSID=$(extract_value "wifiSsid" "$html")
    CURRENT_BROKER=$(extract_value "mqttBroker" "$html")
    CURRENT_PORT=$(extract_value "mqttPort" "$html")
    CURRENT_CLIENT=$(extract_value "mqttClientId" "$html")
    CURRENT_TEMP=$(extract_value "lowTemp" "$html")
    CURRENT_SPEED=$(extract_value "speedPercent" "$html")
    CURRENT_INTERVAL=$(extract_value "sensorInterval" "$html")
}

# ИСПРАВЛЕНО: используем IFS для объединения через '&'
send_post() {
    local data=("$@")
    local IFS='&'
    curl -s -X POST --connect-timeout "$TIMEOUT" "$BASE_URL/save" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        -d "${data[*]}"
}

expect_contains() {
    local expected="$1"
    shift
    local data=("$@")
    
    local response
    response=$(send_post "${data[@]}")
    
    echo "$response" | grep -q "$expected"
}

# ============================================================================
# TESTS
# ============================================================================

run_test() {
    local name="$1"
    local expected="$2"
    shift 2
    local data=("$@")
    
    echo -n "  $name ... "
    
    if expect_contains "$expected" "${data[@]}"; then
        echo -e "${GREEN}PASS${NC}"
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        return 1
    fi
}

restore_config() {
    log_info "Restoring original values..."
    
    local data=(
        "wifiSsid=$CURRENT_SSID"
        "mqttBroker=$CURRENT_BROKER"
        "mqttPort=$CURRENT_PORT"
        "mqttClientId=$CURRENT_CLIENT"
        "sensorInterval=$CURRENT_INTERVAL"
        "confirmSave=1"
        "lowTemp=$CURRENT_TEMP"
        "highTemp=29.0"
        "lowHum=55.0"
        "highHum=60.0"
        "maxOnTime=3600"
        "delaySeconds=60"
        "speedPercent=$CURRENT_SPEED"
    )
    
    send_post "${data[@]}" > /dev/null
    sleep 1
}

# ============================================================================
# MAIN
# ============================================================================

main() {
    echo "=========================================="
    echo "VALIDATION TEST SUITE"
    echo "Target: $BASE_URL"
    echo "=========================================="
    
    check_connection
    
    log_info "Reading current configuration..."
    read_config
    
    echo "  Current values:"
    echo "    SSID: ${CURRENT_SSID:-<empty>}"
    echo "    Broker: ${CURRENT_BROKER:-<empty>}"
    echo "    Port: ${CURRENT_PORT:-<empty>}"
    echo "    Client: ${CURRENT_CLIENT:-<empty>}"
    echo "    Temp: ${CURRENT_TEMP:-<empty>}"
    echo "    Speed: ${CURRENT_SPEED:-<empty>}"
    echo "    Interval: ${CURRENT_INTERVAL:-<empty>}"
    
    local failed=0
    local total=0
    
    echo ""
    log_info "Running tests..."
    
    # Test 1: Valid data
    run_test "Valid data" "successful" \
        "wifiSsid=iot" \
        "mqttBroker=192.168.100.223" \
        "mqttPort=1883" \
        "mqttClientId=fan_F860" \
        "sensorInterval=10" \
        "confirmSave=1" \
        "lowTemp=27.0" \
        "highTemp=29.0" \
        "lowHum=55.0" \
        "highHum=60.0" \
        "maxOnTime=3600" \
        "delaySeconds=60" \
        "speedPercent=50" \
        || ((failed++))
    ((total++))
    
    # Test 2: No confirmSave
    run_test "No confirmSave" "Confirm saving" \
        "wifiSsid=iot" \
        "mqttBroker=192.168.100.223" \
        "mqttPort=1883" \
        "mqttClientId=fan_F860" \
        "sensorInterval=10" \
        "lowTemp=27.0" \
        "speedPercent=50" \
        || ((failed++))
    ((total++))
    
    # Test 3: Invalid port
    run_test "Invalid port (99999)" "MQTT Port" \
        "wifiSsid=iot" \
        "mqttBroker=192.168.100.223" \
        "mqttPort=99999" \
        "mqttClientId=fan_F860" \
        "sensorInterval=10" \
        "confirmSave=1" \
        || ((failed++))
    ((total++))
    
    # Test 4: Empty SSID
    run_test "Empty SSID" "WiFi SSID" \
        "wifiSsid=" \
        "mqttBroker=192.168.100.223" \
        "mqttPort=1883" \
        "mqttClientId=fan_F860" \
        "sensorInterval=10" \
        "confirmSave=1" \
        || ((failed++))
    ((total++))
    
    # Test 5: Invalid temperature
    run_test "Invalid temp (999)" "Low Temp" \
        "wifiSsid=iot" \
        "mqttBroker=192.168.100.223" \
        "mqttPort=1883" \
        "mqttClientId=fan_F860" \
        "sensorInterval=10" \
        "lowTemp=999" \
        "confirmSave=1" \
        || ((failed++))
    ((total++))
    
    # Test 6: Invalid speed
    run_test "Invalid speed (150)" "Speed" \
        "wifiSsid=iot" \
        "mqttBroker=192.168.100.223" \
        "mqttPort=1883" \
        "mqttClientId=fan_F860" \
        "sensorInterval=10" \
        "speedPercent=150" \
        "confirmSave=1" \
        || ((failed++))
    ((total++))
    
    echo ""
    echo "=========================================="
    echo "SUMMARY: $((total - failed))/$total passed"
    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}ALL TESTS PASSED${NC}"
    else
        echo -e "${RED}$failed TESTS FAILED${NC}"
    fi
    echo "=========================================="
    
    restore_config
    
    exit $failed
}

# ============================================================================
# ENTRY POINT
# ============================================================================

main "$@"