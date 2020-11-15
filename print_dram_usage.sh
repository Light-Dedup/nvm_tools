function calc_active_total {
	echo -n "$(($2 * $4)) $(($3 * $4)) "
}
function print_active_total {
	calc_active_total $(sudo grep $1 /proc/slabinfo)
}

# Will reduce by the time.
#print_active_total pmfs_kbuf_cache
print_active_total pmfs_fp_cache
print_active_total pmfs_inner_cache0
print_active_total pmfs_inner_cache1
print_active_total pmfs_inner_cache2
