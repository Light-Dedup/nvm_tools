set -e

if [ ! $2 ]; then
	echo Usage: $0 threads_first total_size\(MiB\)
	exit 1
fi

cd ..
bash setup.sh 1>&2
cd - > /dev/null
each=$(($2 / $1))

echo First:
res1=$(mktemp)
ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1
bash helper/fio.sh $1 ${each}M 0 1>&2
res2=$(mktemp)
ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'
rm $res1 $res2

mkdir /mnt/pmem/0
mv /mnt/pmem/* /mnt/pmem/0 &> /dev/null || true

echo Second:
res1=$(mktemp)
ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1
cp -r /mnt/pmem/0 /mnt/pmem/1
res2=$(mktemp)
ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'
rm $res1 $res2
