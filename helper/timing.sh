dmesg | grep $1 | awk '{print $7}' | tail -n 1 | sed 's/,//g'
