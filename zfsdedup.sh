sudo zpool destroy zfsdedup
sudo zpool create -m /mnt/pmem zfsdedup /dev/pmem0
#sudo zfs set dedup=on zfsdedup
sudo zfs set dedup=edonr,verify zfsdedup
