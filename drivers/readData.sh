#!/bin/bash
set -euo pipefail

# Takes data from the bob and yheets it into a file.
# Usage: readData <tty> <destination>

TTY="$1"
OUTPUT="$2"

echo "r" -> "$TTY"
stdbuf -o0 cat "$TTY" >> "$OUTPUT"

# Remove all '?' characters
sed -i '/?/d' "$OUTPUT"
# Remove empty lines
sed -i '/^$/d' "$OUTPUT"
