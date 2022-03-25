set -e

if [ ! $4 ]; then
	echo Usage: $0 dir thread size dup
	exit
fi

bash timing_concurrent.sh $2 $3 $4
sudo mkdir $1/0/
sudo mv $1/test* $1/0/
time sudo cp -r $1/0/ $1/1
