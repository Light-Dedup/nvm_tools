#!/usr/bin/bash


cd downloads || exit

if [ ! "$(command -v fs-hasher)" ]; then
    wget https://tracer.filesystems.org/fs-hasher-0.9.5.tar.gz
    tar -zxvf fs-hasher-0.9.5.tar.gz
    rm fs-hasher-0.9.5.tar.gz
    cd fs-hasher-0.9.5 || exit
    ./configure
    make
    sudo make install
fi

mkdir -p fslhomes

cd fslhomes || exit

function get2015user () {
    userid=$1
    wget https://tracer.filesystems.org/traces/fslhomes/2015/fslhomes-user"$userid"-2015-04-10.tar.bz2
    tar -xvjf fslhomes-user"$userid"-2015-04-10.tar.bz2
    rm fslhomes-user"$userid"-2015-04-10.tar.bz2
}

get2015user 000
get2015user 002
get2015user 015
get2015user 026
get2015user 027
get2015user 029
get2015user 030
get2015user 031
get2015user 032
get2015user 033
get2015user 034
get2015user 035
get2015user 036
get2015user 037
get2015user 038


