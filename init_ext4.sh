umount /mnt/pmem
mkfs -t ext4 /dev/pmem0
mount -t ext4 /dev/pmem0 /mnt/pmem -o dax

