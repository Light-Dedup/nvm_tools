if [ ! $1 ]; then
	echo Usage: $0 /dev/pmemx
	exit
fi
umount /mnt/pmem
mkfs -t ext4 -F $1
mount -t ext4 $1 /mnt/pmem -o dax

