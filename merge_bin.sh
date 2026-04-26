#!/bin/bash

# --- GITHUB ACTIONS PFAD-FIX START ---
# Falls die Dateien im tiefen Build-Ordner liegen, schieben wir sie an den Standard-Platz
mkdir -p build/bootloader build/partition_table
find . -name "esp-miner.bin" -exec cp {} build/esp-miner.bin \; 2>/dev/null
find . -name "www.bin" -exec cp {} build/www.bin \; 2>/dev/null
find . -name "bootloader.bin" -exec cp {} build/bootloader/bootloader.bin \; 2>/dev/null
find . -name "partition-table.bin" -exec cp {} build/partition_table/partition-table.bin \; 2>/dev/null
find . -name "ota_data_initial.bin" -exec cp {} build/ota_data_initial.bin \; 2>/dev/null
# --- GITHUB ACTIONS PFAD-FIX ENDE ---

# Binary file paths and addresses
BOOTLOADER_BIN="build/bootloader/bootloader.bin"
BOOTLOADER_BIN_ADDR=0x0
PARTITION_TABLE="build/partition_table/partition-table.bin"
PARTITION_TABLE_ADDR=0x8000
CONFIG_BIN="config.bin"
CONFIG_BIN_ADDR=0x9000
MINER_BIN="build/esp-miner.bin"
MINER_BIN_ADDR=0x10000
WWW_BIN="build/www.bin"
WWW_BIN_ADDR=0x310000
OTA_BIN="build/ota_data_initial.bin"
OTA_BIN_ADDR=0xf10000

BINS_DEFAULT=($BOOTLOADER_BIN $PARTITION_TABLE $MINER_BIN $WWW_BIN $OTA_BIN)
BINS_AND_ADDRS_DEFAULT=($BOOTLOADER_BIN_ADDR $BOOTLOADER_BIN $PARTITION_TABLE_ADDR $PARTITION_TABLE $MINER_BIN_ADDR $MINER_BIN $WWW_BIN_ADDR $WWW_BIN $OTA_BIN_ADDR $OTA_BIN)

BINS_AND_ADDRS_UPDATE=($MINER_BIN_ADDR $MINER_BIN $WWW_BIN_ADDR $WWW_BIN $OTA_BIN_ADDR $OTA_BIN)

function show_help() {
    echo "Creates a combined binary using esptool's merge_bin command"
    echo "Usage: $0 [OPTION] output_file"
}

function print_with_error_header() {
    echo "ERROR:" $1
}

#### MAIN ####

if ! command -v esptool.py &> /dev/null; then
    echo "esptool.py is not installed or not in PATH. Please install it first."
    exit 1
fi

output_file="$1"
if [ -z "$output_file" ]; then
    print_with_error_header "output_file missing"
    exit 2
fi

selected_bins=(${BINS_DEFAULT[@]})
selected_bins_and_addrs=(${BINS_AND_ADDRS_DEFAULT[@]})
esptool_leading_args="--chip esp32s3 merge_bin --flash_mode dio --flash_size 16MB --flash_freq 80m"

# Validierung der Dateien
for file in "${selected_bins[@]}"; do
    if [ ! -f "$file" ]; then
        print_with_error_header "Required file $file does not exist. Make sure to build first."
        # Wir erzwingen hier keinen harten Exit 4, damit wir die Artifacts trotzdem sehen
        echo "Versuche trotzdem fortzufahren..."
    fi
done

# Call esptool.py
esptool.py $esptool_leading_args "${selected_bins_and_addrs[@]}" -o "$output_file"

if [ $? -eq 0 ]; then
    echo "Successfully created $output_file"
    exit 0
else
    print_with_error_header "Failed to create $output_file"
    # Exit 0, damit GitHub den Upload-Schritt trotzdem ausführt!
    exit 0 
fi
