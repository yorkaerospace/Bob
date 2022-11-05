#!/bin/bash

# Takes data from the bob and yheets it into a file.
# Usage: readData <tty> <destination>

echo "r" -> $1;
stdbuf -o0 cat $1 >> $2

sed -i '/?/d' $2
sed -i '/^$/d' $2
