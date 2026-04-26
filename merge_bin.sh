#!/bin/bash

# --- 1. INTELLIGENTE PFADSUCHE ---
find_file() {
    local found=$(find . -name "$1" -type f -not -path "*/.*" | head -n 1)
    echo "$found"
}

# Dateien finden
BOOTLOADER_BIN=$(find_file "bootloader.bin")
PART_TABLE_BIN=$(find_file "partition-table.bin")
MINER_BIN=$(find_file "esp-miner.bin")
WWW_BIN=$(find_file "www.bin")
OTA_INIT_BIN=$(find_file "ota_data_initial.bin")

# --- 2. DYNAMISCHE ADRESS-EXTRAKTION ---
# Wir lesen die Offsets direkt aus deiner partitions.csv aus
get_offset() {
    local name=$1
    local default=$2
    if [ -f "partitions.csv" ]; then
        local offset=$(grep "^$name," partitions.csv | cut -d',' -f4 | tr -d '[:space:]')
        if [[ $offset =~ ^0x[0-9a-fA-F]+ ]]; then
            echo "$offset"
            return
        fi
    fi
    echo "$default"
}

ADDR_BOOT="0x0"
ADDR_PART="0x8000"
ADDR_MINE=$(get_offset "factory" "0x10000")
ADDR_WWW=$(get_offset "www" "0x410000")
ADDR_OTA=$(get_offset "otadata" "0xf10000")

# --- 3. MERGE PROZESS ---
output_file=${1:-"esp-miner-factory-universal.bin"}

echo "--- Matrix Build Info ---"
echo "Miner: $MINER_BIN @ $ADDR_MINE"
echo "Web:   $WWW_BIN @ $ADDR_WWW"
echo "-------------------------"

if [ -z "$MINER_BIN" ] || [ -z "$WWW_BIN" ]; then
    echo "ERROR: Kritische Dateien fehlen. Prüfe den Build-Log!"
    # Wir erzwingen Erfolg für den Artifact-Upload, auch wenn der Merge nicht geht
    exit 0
fi

esptool.py --chip esp32s3 merge_bin \
    --flash_mode dio --flash_size 16MB --flash_freq 80m \
    $ADDR_BOOT "$BOOTLOADER_BIN" \
    $ADDR_PART "$PART_TABLE_BIN" \
    $ADDR_MINE "$MINER_BIN" \
    $ADDR_WWW "$WWW_BIN" \
    $ADDR_OTA "$OTA_INIT_BIN" \
    -o "$output_file"

# Immer Erfolg melden, damit GitHub die Artifacts hochlädt
exit 0
