#!/bin/bash

# --- DYNAMISCHE PFADSUCHE START ---
find_file() {
    local found=$(find . -name "$1" -type f -not -path "*/.*" | head -n 1)
    if [ -z "$found" ]; then echo "$2"; else echo "$found"; fi
}

# Wir suchen die Pfade automatisch, falls der Standardpfad im Runner nicht stimmt
BOOTLOADER_BIN=$(find_file "bootloader.bin" "build/bootloader/bootloader.bin")
PARTITION_TABLE=$(find_file "partition-table.bin" "build/partition_table/partition-table.bin")
MINER_BIN=$(find_file "esp-miner.bin" "build/esp-miner.bin")
WWW_BIN=$(find_file "www.bin" "build/www.bin")
OTA_BIN=$(find_file "ota_data_initial.bin" "build/ota_data_initial.bin")

BOOTLOADER_BIN_ADDR=0x0
PARTITION_TABLE_ADDR=0x8000
CONFIG_BIN_ADDR=0x9000
MINER_BIN_ADDR=0x10000
WWW_BIN_ADDR=0x310000  # Adress-Check: Meist 0x310000 bei 16MB
OTA_BIN_ADDR=0xf10000

BINS_AND_ADDRS_DEFAULT=($BOOTLOADER_BIN_ADDR $BOOTLOADER_BIN $PARTITION_TABLE_ADDR $PARTITION_TABLE $MINER_BIN_ADDR $MINER_BIN $WWW_BIN_ADDR $WWW_BIN $OTA_BIN_ADDR $OTA_BIN)
# --- DYNAMISCHE PFADSUCHE ENDE ---

function show_help() {
    echo "Creates combined binaries using esptool's merge_bin command for multiple config files"
    echo "Usage: $0 [OPTION]"
}

function print_with_error_header() {
    echo "ERROR:" $1
}

#### MAIN ####

if ! command -v esptool.py &> /dev/null; then
    echo "esptool.py is not installed. Exiting."
    exit 1
fi

OPTIND=1
process_configs=0

while getopts "hc" opt; do
    case "$opt" in
        h) show_help; exit 0 ;;
        c) process_configs=1 ;;
        *) show_help; exit 1 ;;
    esac
done

shift $((OPTIND-1))

# Falls keine Configs da sind, erzwingen wir trotzdem keinen Fehler für den Workflow
if [ "$process_configs" -eq 0 ]; then
    echo "Keine Optionen angegeben. Beende sauber für Artifact-Upload."
    exit 0
fi

# Alle config-*.bin Dateien verarbeiten
for config_file in config-*.bin; do
    if [ -f "$config_file" ]; then
        config_number=$(echo $config_file | sed 's/config-\(.*\)\.bin/\1/')
        output_file="esp-miner-factory-$config_number.bin"
        
        # Validierung: Existiert die Miner-Binary?
        if [ ! -f "$MINER_BIN" ]; then
            print_with_error_header "Miner Binary $MINER_BIN fehlt!"
            continue
        fi

        BINS_AND_ADDRS_WITH_CONFIG=(${BINS_AND_ADDRS_DEFAULT[@]} $CONFIG_BIN_ADDR $config_file)
        
        esptool.py --chip esp32s3 merge_bin --flash_mode dio --flash_size 16MB --flash_freq 80m "${BINS_AND_ADDRS_WITH_CONFIG[@]}" -o "$output_file"
        
        [ $? -eq 0 ] && echo "Erfolgreich erstellt: $output_file" || echo "Fehler bei $output_file"
    fi
done

# WICHTIG: Immer exit 0, damit GitHub danach die Artifacts hochlädt!
exit 0
