if [ ! $1 ]; then
 target_dev=/dev/pmem0
else
 target_dev=$1
fi

umount /mnt/pmem
mkfs -t ext4 -F $target_dev 2>&1
mount -t ext4 $target_dev /mnt/pmem -o dax

