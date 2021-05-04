sudo umount /mnt/pmem
sudo zpool destroy zfsdedup
sudo zpool create -m /mnt/pmem zfsdedup /dev/pmem0
sudo zfs set recordsize=4K dedup=edonr,verify zfsdedup
