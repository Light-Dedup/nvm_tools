if [ $(whoami) != "root" ]; then
	echo Please run as root.
	exit
fi
bash latency_table.sh $* > output.txt
