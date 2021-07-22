#!/bin/python3

import os, sys, shutil

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: ' + sys.argv[0] + ' num target_dir')
        exit(0)
    num = int(sys.argv[1], 10)
    target_dir = sys.argv[2]
    total_sz = 0
    files = []
    for file in sys.stdin:
        file = file.strip()
        total_sz += os.path.getsize(file)
        files.append(file)
    split_sz = total_sz / num
    cur_sz = 0
    cur_num = 0
    cur_dir = target_dir + '/' + str(cur_num) + '/'
    for file in files:
        cur_sz += os.path.getsize(file)
        target = cur_dir + file
        dir = os.path.dirname(target)
        os.makedirs(dir, exist_ok = True)
        shutil.copyfile(file, target)
        if cur_sz >= split_sz:
            cur_sz = 0
            cur_num += 1
            cur_dir = target_dir + '/' + str(cur_num) + '/'
