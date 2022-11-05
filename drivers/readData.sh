#!/bin/bash

echo "r" -> /dev/ttyACM0;
stdbuf -o0 cat /dev/ttyACM0 >> log.csv

