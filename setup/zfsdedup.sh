set -e
if [ $(whoami) != "root" ]; then
	echo Please run as root
	exit
fi
umount /mnt/pmem 2> /dev/null || true
rm /mnt/pmem/pmem_hash.data || true
zpool destroy zfsdedup 2> /dev/null || true
zpool create -m /mnt/pmem zfsdedup /dev/pmem0
zfs set recordsize=4K dedup=on zfsdedup
