cd ..
cd /mnt
sudo bash init_ext4.sh
cd -

cd /mnt/pmem
# Warm up
for i in $(seq 1 $1); do
	nohup sudo fio -filename=./test$i -direct=1 -iodepth 1 -rw=write -ioengine=mmap -bs=256M -thread -numjobs=1 -size=256M -name=randrw --dedupe_percentage=0 -group_reporting > /dev/null &
done
wait

TMPFILE=$(mktemp)
for i in $(seq 1 $1); do
	nohup sudo fio -filename=./test$i -direct=1 -iodepth 1 -rw=write -ioengine=mmap -bs=256M -thread -numjobs=1 -size=256M -name=randrw --dedupe_percentage=0 -group_reporting >> $TMPFILE &
done
wait
cd -

make
echo -n Write:
grep WRITE: $TMPFILE | sed 's/.*WRITE: bw=//g' | sed 's/iB.*//g' | ./M2G | ./get_sum | tr -d "\n"
echo GiB/s

rm $TMPFILE
TMPFILE=$(mktemp)
cd -
for i in $(seq 1 $1); do
	nohup sudo fio -filename=./test$i -direct=1 -iodepth 1 -rw=read -ioengine=mmap -bs=256M -thread -numjobs=1 -size=256M -name=randrw --dedupe_percentage=0 -group_reporting >> $TMPFILE &
done
wait
cd -

echo -n Read:
grep READ: $TMPFILE | sed 's/.*READ: bw=//g' | sed 's/iB.*//g' | ./M2G | ./get_sum | tr -d "\n"
echo GiB/s

rm $TMPFILE

