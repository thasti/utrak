#!/bin/bash

mspdebug rf2500 "prog main.elf" # &> debug.log &
#mspdebug rf2500 "verify main.elf" # &> debug.log &
#read -n 1
#mspdebug rf2500 "gdb" &> build/debug.log &
#cgdb -d msp430-gdb main.elf 
# echo target remote localhost:2000 | 
# msp430-gdb main.elf
