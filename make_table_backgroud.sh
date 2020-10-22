if [ $(whoami) != "root" ]; then
	echo Please run as root.
	exit
fi
nohup bash make_table.sh $* > output.txt &

