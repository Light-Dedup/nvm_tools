dmesg | grep $1 | sed 's/\[//g' | awk '{print $3,$5,$7}' | tail -n 1 | sed 's/,//g' | sed 's/://g'
