cd ..
make -j$(nproc)
bash setup.sh
cd -
sudo fio -filename=/mnt/pmem/test1 -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size=200G -name=test --dedupe_percentage=0 -group_reporting -write_bw_log=test -write_lat_log=test -write_iops_log=test -log_avg_msec=1000
