if [ $(whoami) != "root" ]; then
	echo Please run as root.
	exit
fi
bash make_table.sh $* > output.txt

