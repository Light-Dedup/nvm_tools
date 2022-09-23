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

function nova_attr_time_stats () {
    ATTR=$1
    TARGET_STATS=$2
    
    CMD="awk '\$1==\"ATTR:\" {print \$5}' $TARGET_STATS"
    echo "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/ATTR/${ATTR}/g" /tmp/awk_nova_attr_time
    CMD=$(cat /tmp/awk_nova_attr_time)
    
    bash -c "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/,//g" /tmp/awk_nova_attr_time
    cat /tmp/awk_nova_attr_time
    rm /tmp/awk_nova_attr_time
}

branch_name=$4
pmem_id=$7

git checkout "$branch_name"

sudo make -j32
sudo bash -c "echo $0 $* > /dev/kmsg"

restore_pmem "setup.sh" 
set_pmem "setup.sh" "pmem0"
sudo bash setup.sh 1
restore_pmem "setup.sh" 

git checkout -- "setup.sh"

# phase 1
res1=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1

sudo "$ABSPATH"/aging_system -d /mnt/pmem0 -s $5 -o $6 -p 1

cat /proc/fs/NOVA/pmem0/timing_stats > "$ABSPATH"/Newly-"$branch_name"
newly_dedup_cost=$(nova_attr_time_stats "incr_continuous" "$ABSPATH"/Newly-"$branch_name")   

res2=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'

# phase 2
sudo "$ABSPATH"/aging_system -d /mnt/pmem0 -s $5 -o $6 -p 2
# ino=$(stat /mnt/pmem0/file1 | grep "Inode" | awk '{print $2}' | tr -cd "[0-9]")
# echo "$ino" > /proc/fs/NOVA/pmem0/gc

# phase 3
res1=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1

echo 1 > /proc/fs/NOVA/pmem0/timing_stats

sudo "$ABSPATH"/aging_system -d /mnt/pmem0 -s $5 -o $6 -p 3

cat /proc/fs/NOVA/pmem0/timing_stats > "$ABSPATH"/Aging-"$branch_name"
aging_dedup_cost=$(nova_attr_time_stats "incr_continuous" "$ABSPATH"/Aging-"$branch_name")

res2=$(mktemp)
sudo ipmctl show -dimm $pmem_id -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'
rm $res1 $res2
echo "NewlyDedupCost: $newly_dedup_cost"
echo "AgingDedupCost: $aging_dedup_cost"
cd - > /dev/null