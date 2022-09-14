if [ ! $1 ]; then
	echo Usage: $0 dir
	exit 1
fi

function check {
	res="$(cat $1/hello)"
	if [ "$res" != "$2" ]; then
		echo Correction test of writing small content in $1 failed. Expected "$2", found "$res".
		exit 1
	fi
}

echo -n "Hello" > $1/hello
check $1 Hello
echo " world" >> $1/hello
check $1 "Hello world"
