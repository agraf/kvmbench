BEFORE=$(C=0; for i in $(grep RES: /proc/interrupts | cut -d : -f 2 | cut -d R -f 1); do C=$(( $C + $i )); done; echo $C)

./l3cache

AFTER=$(C=0; for i in $(grep RES: /proc/interrupts | cut -d : -f 2 | cut -d R -f 1); do C=$(( $C + $i )); done; echo $C)

echo "Nr of RES IPIs: " $(( AFTER - BEFORE )) 
