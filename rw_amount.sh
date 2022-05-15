set -e

cd ..
make -j$(nproc)
bash setup.sh
cd - > /dev/null
res1=$(mktemp)
sudo ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res1
bash helper/fio.sh $*
res2=$(mktemp)
sudo ipmctl show -dimm 0x20 -performance | grep TotalMedia | awk -F= '{print $1,$2}' | sed 's/.*Total//g' > $res2
paste $res1 $res2 | awk --non-decimal-data '{print $1,($4-$2)*64}'
rm $res1 $res2
