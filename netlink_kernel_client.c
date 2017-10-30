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

#define NETLINK_TEST     30
#define MSG_LEN            125
#define USER_PORT        100

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhangwj");
MODULE_DESCRIPTION("netlink example");

enum {
	NLK_DPI = 0,
	NLK_WEBSERVER,
	NLK_GATEWAY
};

struct sock *nlsk = NULL;
extern struct net init_net;
static int i;
static struct timer_list zc_timer;
static int send_to_user(int index);

int grp = 1;
module_param(grp,int,S_IRUSR);

int send_usrmsg(char *pbuf, uint16_t len)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;

    int ret;

    /* 创建sk_buff 空间 */
    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if(!nl_skb)
    {
        printk("netlink alloc failure\n");
        return -1;
    }

    /* 设置netlink消息头部 */
    nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_TEST, len, 0);
    if(nlh == NULL)
    {
        printk("nlmsg_put failaure \n");
        nlmsg_free(nl_skb);
        return -1;
    }

    /* 拷贝数据发送 */
    memcpy(nlmsg_data(nlh), pbuf, len);
    ret = netlink_unicast(nlsk, nl_skb, USER_PORT, MSG_DONTWAIT);

    return ret;
}




static void netlink_rcv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    char *umsg = NULL;


    if(skb->len >= nlmsg_total_size(0))
    {
        nlh = nlmsg_hdr(skb);
        umsg = NLMSG_DATA(nlh);
        if(umsg)
        {
            printk("kernel recv from user: %s\n", umsg);
//            send_usrmsg(kmsg, strlen(kmsg));
        }
    }
}


struct netlink_kernel_cfg cfg = { 
        .input  = netlink_rcv_msg, /* set recv callback */
		.groups = 1 << NLK_DPI
};  

static void timer_do_func(unsigned long arg)
{
	zc_timer.expires = jiffies + 5 * HZ;
	i++;
	printk(KERN_DEBUG "now is %d ... %d\n", i, grp);
	send_to_user(i);
	add_timer(&zc_timer);
	return;
}




static int send_to_user(int index)
{
	struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
	char msg[20] = "Hello from kernel";
    int ret;

    /* 创建sk_buff 空间 */
    nl_skb = nlmsg_new(MSG_LEN, GFP_ATOMIC);
    if(!nl_skb)
    {
        printk("netlink alloc failure\n");
        return -1;
    }

    /* 设置netlink消息头部 */
    nlh = nlmsg_put(nl_skb, 0, 0, NLMSG_DONE, MSG_LEN, 0);
    if(nlh == NULL)
    {
        printk("nlmsg_put failaure \n");
        nlmsg_free(nl_skb);
        return -1;
    }

    /* 拷贝数据发送 */
    memcpy(nlmsg_data(nlh), msg, sizeof(msg));
	memset(nlmsg_data(nlh)+strlen(msg), grp+48, 1);
	
    ret = nlmsg_multicast(nlsk, nl_skb, 0, grp, MSG_DONTWAIT);
	if (ret < 0) 
		printk(KERN_INFO "Error while sending bak to user, err id: %d\n", ret);
	return 0;
}

int test_netlink_init(void)
{
    /* create netlink socket */
    nlsk = (struct sock *)netlink_kernel_create(&init_net, NETLINK_TEST, &cfg);
    if(nlsk == NULL)
    {   
        printk("netlink_kernel_create error !\n");
        return -1; 
    }   
    printk("test_netlink_init\n");
	init_timer(&zc_timer);
	zc_timer.function = timer_do_func;
	zc_timer.expires = jiffies + 10 * HZ;
	add_timer(&zc_timer);
	printk(KERN_DEBUG "timer add ...\n");
	i = 0;
    return 0;
}

void test_netlink_exit(void)
{
	del_timer(&zc_timer);
    if (nlsk){
        netlink_kernel_release(nlsk); /* release ..*/
        nlsk = NULL;
    }   
    printk("test_netlink_exit!\n");
}

module_init(test_netlink_init);
module_exit(test_netlink_exit);

