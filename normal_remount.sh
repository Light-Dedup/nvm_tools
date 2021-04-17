set -e
cd ..
sudo umount /mnt/pmem || true
sudo rmmod nova || true
sudo insmod nova.ko measure_timing=1
sudo mount -t NOVA -o wprotect,data_cow /dev/pmem0 /mnt/pmem
sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test > /dev/null
dmesg | grep normal_recover_fp_table | tail -n 1
dmesg | grep normal_recover_entry_allocator | tail -n 1
sudo umount /mnt/pmem
sudo mount -t NOVA -o wprotect,data_cow /dev/pmem0 /mnt/pmem
sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test > /dev/null
dmesg | grep save_refcount | tail -n 1
dmesg | grep save_entry_allocator | tail -n 1
