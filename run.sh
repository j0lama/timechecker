#!/bin/bash

# Check root permissions
if [ "$(id -u)" -ne 0 ]; then
    echo "You must run with sudo"
    exit 1
fi

if [ "$#" -ne 2 ]; then
    echo "USE: sudo $0 <CPU ID> <Threshold (usec)>"
    exit 1
fi

# Disable the softlockup threshold by setting it to 0 (Default: 10)
echo "0" | sudo tee /proc/sys/kernel/watchdog_thresh > /dev/null

sudo insmod timechecker.ko cpu_id=$1 threshold=$2
