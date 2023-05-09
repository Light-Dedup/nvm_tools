if [[ $# != 1 ]]; then
	echo $0 pmem-num
fi

umount /mnt/pmem$1
mkfs -t ext4 -F /dev/pmem$1 2>&1
mount -t ext4 /dev/pmem$1 /mnt/pmem$1 -o dax
