set -e

if [ ! $1 ]; then
	echo Usage: $0 size
	exit 1
fi

function run {
	res1=$(mktemp)
	sudo ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1
	sudo fio -filename=$1 -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size=$2 -name=test --dedupe_percentage=0 -group_reporting 1>&2
	res2=$(mktemp)
	sudo ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
	paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'
	rm $res1 $res2
}

cd ..
make -j$(nproc) 1>&2
sudo bash setup.sh 1>&2
cd - > /dev/null
echo First:
run /mnt/pmem/test1 $1
echo Second:
run /mnt/pmem/test2 $1
