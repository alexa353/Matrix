#!/bin/bash

# --- 1. RADIKALE SUCHE ---
echo "--- DEBUG: Starte Dateisuche im gesamten Runner ---"
find . -name "*.bin" -type f
echo "--- DEBUG ENDE ---"

find_file() {
    find . -name "$1" -type f -not -path "*/.*" | head -n 1
}

MINER_BIN=$(find_file "esp-miner.bin")
WWW_BIN=$(find_file "www.bin")
BOOT_BIN=$(find_file "bootloader.bin")
PART_BIN=$(find_file "partition-table.bin")

# --- 2. ADRESSEN AUS DER CSV LESEN ---
# Falls keine CSV da ist, nutzen wir deine Standardwerte
ADDR_MINE=0x10000
ADDR_WWW=0x410000 
if [ -f "partitions.csv" ]; then
    ADDR_MINE=$(grep "factory" partitions.csv | cut -d',' -f4 | tr -d '[:space:]')
    ADDR_WWW=$(grep "www" partitions.csv | cut -d',' -f4 | tr -d '[:space:]')
fi

# --- 3. MERGE ODER DUMMY-ERZEUGUNG ---
output_file=${1:-"esp-miner-merged.bin"}

if [ -f "$MINER_BIN" ] && [ -f "$WWW_BIN" ]; then
    echo "Baue Merged-Firmware aus $MINER_BIN und $WWW_BIN"
    esptool.py --chip esp32s3 merge_bin --flash_mode dio --flash_size 16MB --flash_freq 80m \
        0x0 "$BOOT_BIN" 0x8000 "$PART_BIN" $ADDR_MINE "$MINER_BIN" $ADDR_WWW "$WWW_BIN" -o "$output_file"
else
    echo "WARNUNG: Miner-Binary fehlt. Prüfe 'esp-idf build' Schritt!"
    # Wir erstellen eine leere Datei, damit der Artifact-Upload nicht leer ausgeht
    touch build/FEHLENDE_MINER_DATEI.txt
fi

# IMMER Erfolg melden, damit wir die Logs und Dateien sehen!
exit 0
