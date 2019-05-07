#!/bin/sh
make clean;
make;
boot/boot minikernel/kernel > output.txt;