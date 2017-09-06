#!/bin/bash

if [ ! -x /usr/bin/john ]; then
    echo "Please install john, for example from"
    echo
    echo "  http://download.opensuse.org/repositories/security/SLE_12_SP2"
    exit 1
fi

function expand_range() {
    if echo "$1" | grep -q -; then
        for i in $(seq $(echo "$1" | cut -d - -f 1) $(echo "$1" | cut -d - -f 2)); do
            echo -n "$i "
        done
    else
        echo "$1"
    fi
}

# Find HT companion
CPU0=$(expand_range $(sed 's/,/ /g' /sys/devices/system/cpu/cpu0/topology/thread_siblings_list))
CPU2=$(expand_range $(sed 's/,/ /g' /sys/devices/system/cpu/cpu2/topology/thread_siblings_list))

echo "Running on CPUs 0 and 2:"
for i in 0 2; do
    ( taskset -c $i john --format=raw-md5 -test 2>&1 | grep real | cut -d : -f 2 | cut -d c -f 1 ) &
done
wait

echo
echo "Including HTs (CPUs $CPU0 $CPU2):"
for i in $CPU0 $CPU2; do
    ( taskset -c $i john --format=raw-md5 -test 2>&1 | grep real | cut -d : -f 2 | cut -d c -f 1 ) &
done
wait
