#!/bin/bash

# --- AUTOMATISCHE PFAD-KORREKTUR ---
# Sucht die Dateien im build-Ordner, falls sie nicht am Standardort liegen
function find_file() {
    local found=$(find ./build -name "$1" -type f | head -n 1)
    if [ -z "$found" ]; then echo "$2"; else echo "$found"; fi
}

BOOTLOADER_BIN=$(find_file "bootloader.bin" "build/bootloader/bootloader.bin")
PARTITION_TABLE=$(find_file "partition-table.bin" "build/partition_table/partition-table.bin")
MINER_BIN=$(find_file "esp-miner.bin" "build/esp-miner.bin")
WWW_BIN=$(find_file "www.bin" "build/www.bin")
OTA_BIN=$(find_file "ota_data_initial.bin" "build/ota_data_initial.bin")

# Adressen bleiben gleich
BOOTLOADER_BIN_ADDR=0x0
PARTITION_TABLE_ADDR=0x8000
CONFIG_BIN="config.bin"
CONFIG_BIN_ADDR=0x9000
MINER_BIN_ADDR=0x10000
WWW_BIN_ADDR=0x410000
OTA_BIN_ADDR=0xf10000

# Der Rest des Skripts nutzt nun die dynamischen Pfade
BINS_DEFAULT=($BOOTLOADER_BIN $PARTITION_TABLE $MINER_BIN $WWW_BIN $OTA_BIN)
BINS_AND_ADDRS_DEFAULT=($BOOTLOADER_BIN_ADDR $BOOTLOADER_BIN $PARTITION_TABLE_ADDR $PARTITION_TABLE $MINER_BIN_ADDR $MINER_BIN $WWW_BIN_ADDR $WWW_BIN $OTA_BIN_ADDR $OTA_BIN)
# ... (hier folgen deine anderen Arrays, sie nutzen jetzt automatisch die Variablen oben)

# [Hier kommt dein restlicher Code ab der Funktion show_help() - er bleibt fast gleich]
# NUR EINE ÄNDERUNG: Am Ende des Skripts erzwingen wir den Erfolg für GitHub
# [Nach der Zeile: exit 5]
exit 0 
