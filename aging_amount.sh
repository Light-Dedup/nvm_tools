set -e


if [ ! $3 ]; then
	echo Usage: $0 num_of_threads size dup_rate branch_name aging_size aging_hole pmem_id
	exit 1
fi

ABSPATH=$(cd "$( dirname "$0" )" && pwd)

cd "$ABSPATH"/../Light-Dedup || exit

function set_pmem () {
    local script=$1
    local pmem=$2
    
    cp "$script" "/tmp/setups" -f
    sed -i "s/pmem0/pmem/g" "$script"
    sed -i "s/pmem/$pmem/g" "$script"
}

function restore_pmem () {
    local script=$1
    
    cp "/tmp/setups" "$script" -f
}

branch_name=$4
pmem_id=$7

git checkout "$branch_name"

sudo make -j32
sudo bash -c "echo $0 $* > /dev/kmsg"

restore_pmem "setup.sh" 
set_pmem "setup.sh" "pmem0"
sudo bash setup.sh 0
restore_pmem "setup.sh" 

git checkout -- "setup.sh"

# phase 1
res1=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1

sudo "$ABSPATH"/aging_system -d /mnt/pmem0 -s $5 -o $6 -p 1

res2=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'

# phase 2
sudo "$ABSPATH"/aging_system -d /mnt/pmem0 -s $5 -o $6 -p 2

# phase 3
res1=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1

sudo "$ABSPATH"/aging_system -d /mnt/pmem0 -s $5 -o $6 -p 3

res2=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'
rm $res1 $res2
cd - > /dev/null