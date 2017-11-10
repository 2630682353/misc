#!/bin/bash
while((0<1))
do
	`free | grep Mem >> mem_log`
	`cat /proc/slabinfo | grep buffer_head >> mem_log`
	`cat /proc/slabinfo | grep dentry >> mem_log`
	`cat /proc/meminfo | grep Slab >> mem_log`
	`echo "  " >> mem_log`
	sleep 60
done
