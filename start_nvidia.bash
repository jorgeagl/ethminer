#!/bin/bash
export GPU_MAX_HEAP_SIZE=100
export GPU_USE_SYNC_OBJECTS=1
export GPU_MAX_ALLOC_PERCENT=100
screen -dmS ethminer ./ethminer -U -RH -HWMON -S eu1.ethermine.org:4444 -O 0x8FB24C5b5a75887b429d886DBb57fd053D4CF3a2.rig1:x --farm-recheck 5000 --api-port -3333 -FS eu2.ethermine.org:4444 --cuda-parallel-hash 2 --cuda-grid-size 4096
echo "Started ethminer in screen session, use screen -r ethminer to attach"
