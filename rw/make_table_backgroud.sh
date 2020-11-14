if [ $(whoami) != "root" ]; then
	echo Please run as root.
	exit
fi
echo Please enter the times of repeatition:
read repeat

nohup bash -c "
	for i in \$(seq 1 $repeat); do
		bash make_table.sh $* > output_\$i.txt
	done
" &

