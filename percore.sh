set -e

if [ ! $4 ]; then
	echo Usage: $0 dir thread size dup
	exit
fi

# To prevent password
sudo echo
start_time=$(date +%s%N)
for i in $(seq 1 $2); do
	sudo fio -filename=$1/test$i -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size=$3 -name=test --dedupe_percentage=$4 -group_reporting -randseed=$i &
done
wait
end_time=$(date +%s%N)
first=$(($end_time - $start_time))

sudo mkdir $1/0/
sudo mv $1/test* $1/0/
start_time=$(date +%s%N)
for i in $(seq 1 $2); do
	sudo fio -filename=$1/test$i -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size=$3 -name=test --dedupe_percentage=$4 -group_reporting -randseed=$i
done
end_time=$(date +%s%N)
second=$(($end_time - $start_time))

echo First: $first ns
echo Second: $second ns
