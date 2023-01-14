#!/bin/sh

cd $(dirname "$0")

./test.elf &> "$SDCARD_PATH/log.txt"
