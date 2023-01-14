#!/bin/sh

ELF=keymon.elf

# killall --help

# killall -l

killall -STOP $ELF
sleep 5
killall -CONT $ELF