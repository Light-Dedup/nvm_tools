cd ..
bash setup.sh
cd /mnt/pmem
sudo bash -c 'echo hello > hello'
sudo bash -c 'echo First hello done > /dev/kmsg'
sudo bash -c 'echo 233 >> hello'
cat hello
