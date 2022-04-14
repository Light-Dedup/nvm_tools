set -e

if [ ! $4 ]; then
	echo Usage: $0 dir thread size_per_thread\(GiB\) pmem_id
	exit
fi

ABSPATH=$(cd "$( dirname "$0" )" && pwd)
pmem_id=$4
# To prevent password
# sudo echo
start_time=$(date +%s%N)
res1=$(mktemp)
sudo ipmctl show -dimm "$pmem_id" -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > "$res1"
for i in $(seq 0 $((8-1))); do
	for j in $(seq 0 $((16-1))); do
		id=$(($i*16+$j))
		sudo "$ABSPATH"/../nvm_tools/write_1G $id &
	done
	wait
done
res2=$(mktemp)
sudo ipmctl show -dimm "$pmem_id" -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > "$res2"
OUTPUT=$(paste "$res1" "$res2" | awk --non-decimal-data '{print $1,($4-$2)*64}')
rm "$res1" "$res2"
first_read=$(echo "$OUTPUT" | grep MediaReads | awk '{print $2}')
first_write=$(echo "$OUTPUT" | grep MediaWrites | awk '{print $2}')
end_time=$(date +%s%N)
first=$(($end_time - $start_time))

sudo mkdir $1/0
sudo mv $1/test* $1/0/
start_time=$(date +%s%N)
res1=$(mktemp)
sudo ipmctl show -dimm "$pmem_id" -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > "$res1"

for i in $(seq 0 $(($3-1))); do
	for j in $(seq 0 $(($2-1))); do
		id=$(($j*$3+$i))
		sudo "$ABSPATH"/../nvm_tools/write_1G $id &
	done
	wait
done
res2=$(mktemp)
sudo ipmctl show -dimm "$pmem_id" -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > "$res2"
OUTPUT=$(paste "$res1" "$res2" | awk --non-decimal-data '{print $1,($4-$2)*64}')
rm "$res1" "$res2"
second_read=$(echo "$OUTPUT" | grep MediaReads | awk '{print $2}')
second_write=$(echo "$OUTPUT" | grep MediaWrites | awk '{print $2}')
end_time=$(date +%s%N)
second=$(($end_time - $start_time))

echo FirstTime: $first ns
echo SecondTime: $second ns
echo FirstWrite: "$first_write" B
echo FirstRead: "$first_read" B
echo SecondWrite: "$second_write" B
echo SecondRead: "$second_read" B
