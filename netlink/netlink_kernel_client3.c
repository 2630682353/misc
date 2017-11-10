/****************************************
* Author: zhangwj
* Date: 2017-01-19
* Filename: netlink_test.c
* Descript: netlink of kernel
* Kernel: 3.10.0-327.22.2.el7.x86_64
* Warning:
******************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/kthread.h>   
#include "message.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhangwj");
MODULE_DESCRIPTION("netlink example");


static struct task_struct *tsk;  

extern struct net init_net;

static struct timer_list zc_timer;
extern int msg_send_syn(int32 cmd, void *sbuf, int slen, void **obuf, int *olen);
extern int32 msg_cmd_register(const int32 cmd, msg_cmd_handle_cb cb);
extern int32 msg_cmd_unregister(const int32 cmd);
extern int32 free_rcv_buf(void *rcv_buf);



static int thread_function(void *data)  
{  
	char temp_buf[1000] = {0};
    unsigned int success = 1;
	unsigned int error = 0;
	unsigned int notcmp = 0;
	int ilen = 0;
	int olen = 0;
	char *out = NULL;
	int temp = 0;
    do {  
//      printk(KERN_INFO "thread_function: %d times", ++time_count);  
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(0.002*HZ);
		__set_current_state(TASK_RUNNING);
		
		ilen = jiffies%999 + 1;
		memset(temp_buf, jiffies%10, ilen - 1);
		temp = success%5000;
		if (temp < 5)
			printk("DPI success:%d, notcmp:%d, error:%d\n", success, notcmp, error);
		
		if (msg_send_syn(GATEWAY_TEST1, temp_buf, ilen, &out, &olen) != 0)
			error++;
		else {
			if((memcmp(temp_buf, out, ilen) == 0) && (ilen == olen))
				success++;
			else
				notcmp++;
			free_rcv_buf(out);
		}

//		__set_current_state(TASK_INTERRUPTIBLE);
//		schedule_timeout(0.05*HZ);
//		__set_current_state(TASK_RUNNING);
		
		if (msg_send_syn(DPI_TEST1, temp_buf, ilen, &out, &olen) != 0)
			error++;
		else {
			if((memcmp(temp_buf, out, ilen) == 0) && (ilen == olen))
				success++;
			else
				notcmp++;
			free_rcv_buf(out);
		}

		if (msg_send_syn(GATEWAY_KERNEL_TEST1, temp_buf, ilen, &out, &olen) != 0)
			error++;
		else {
			if((memcmp(temp_buf, out, ilen) == 0) && (ilen == olen))
				success++;
			else
				notcmp++;
			free_rcv_buf(out);
		}

		if (msg_send_syn(WS_TEST1, temp_buf, ilen, &out, &olen) != 0)
			error++;
		else {
			if((memcmp(temp_buf, out, ilen) == 0) && (ilen == olen))
				success++;
			else
				notcmp++;
			free_rcv_buf(out);
		}

		if (msg_send_syn(WS_KERNEL_TEST1, temp_buf, ilen, &out, &olen) != 0)
			error++;
		else {
			if((memcmp(temp_buf, out, ilen) == 0) && (ilen == olen))
				success++;
			else
				notcmp++;
			free_rcv_buf(out);
		}
	
    }while(!kthread_should_stop());  
    return 0;  
} 

int32 handle_test2(const int32 cmd, void *ibuf, int32 ilen, void *obuf, int32 *olen)
{
	
//	printk(KERN_DEBUG "handle_test3 %d : %s\n", ilen, (char*)ibuf);
	memcpy(obuf, ibuf, ilen);
	*olen = ilen;
	return 0;
}


static void timer_do_func(unsigned long arg)
{
	zc_timer.expires = jiffies + 5 * HZ;
	add_timer(&zc_timer);
	return;
}


int test_netlink_init(void)
{

	msg_cmd_register(DPI_KERNEL_TEST1, handle_test2);
/*
	init_timer(&zc_timer);
	zc_timer.function = timer_do_func;
	zc_timer.expires = jiffies + 5 * HZ;
	add_timer(&zc_timer);
	printk(KERN_DEBUG "timer add ...\n");
	i = 0;
*/
	tsk = kthread_run(thread_function, NULL, "mythread%d", 1);  
    if (IS_ERR(tsk)) {  
        printk(KERN_INFO "create kthread failed!\n");  
    }  
    else {  
        printk(KERN_INFO "create ktrhead ok!\n");  
    }  
 
    return 0;
}

void test_netlink_exit(void)
{
//	del_timer(&zc_timer);
    msg_cmd_unregister(DPI_KERNEL_TEST1);
    printk("test_netlink_exit!\n");
	if (!IS_ERR(tsk)){  
        int ret = kthread_stop(tsk);  
        printk(KERN_INFO "thread function has run %ds\n", ret);  
    }  
}

module_init(test_netlink_init);
module_exit(test_netlink_exit);


