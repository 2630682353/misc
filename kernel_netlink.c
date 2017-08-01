#include <linux/module.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>
#include <linux/timer.h>
#define NETLINK_TEST 31

#define NLMSG_SETECHO 0x11
#define NLMSG_GETECHO 0x12

static void timer_do_func(unsigned long arg);

static struct sock *sk; //内核端socket
static void nl_custom_data_ready(struct sk_buff *skb);//接收消息回调函数
static struct timer_list zc_timer;
static int i;


int __init nl_custom_init(void)
{
	struct netlink_kernel_cfg nlcfg = {
		.input = nl_custom_data_ready,
	};
	sk = netlink_kernel_create(&init_net, NETLINK_TEST, &nlcfg);
	printk(KERN_DEBUG  "initialed ok!\n");
	if (!sk) {
		printk(KERN_DEBUG  "netlink create error!\n");
	}
	// init_timer(&zc_timer);
	// zc_timer.function = timer_do_func;
	// zc_timer.expires = jiffies + 10 * HZ;
	// add_timer(&zc_timer);
	// printk(KERN_DEBUG "timer add ...\n");
	// i = 0;
	return 0;
}

static void timer_do_func(unsigned long arg)
{
	zc_timer.expires = jiffies + 10 * HZ;
	i++;
	printk(KERN_DEBUG "now is %d ...\n", i);

	// struct sk_buff *out_skb;
	// void *out_payload;
	// struct nlmsghdr *out_nlh;
	// int payload_len = NLMSG_SPACE(MAX_PAYLOAD);
	// out_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);//分配足以存放默认大小的sk_buff
	// if (!out_skb) goto failure;
	// out_nlh = nlmsg_put(out_skb, 0, 0, NLMSG_SETECHO, payload_len, 0);
	// if (!out_nlh) goto failure;
	// out_payload = nlmsg_data(out_nlh);
	// strcpy(out_payload, "hello world zczczc");
	//strcat(out_payload, payload);
	//nlmsg_unicast(sk, out_skb, 12345);
	add_timer(&zc_timer);
	return;
failure:
	printk(KERN_DEBUG  "failed in fun dataready\n");
}

void __exit nl_custom_exit(void)
{
	printk(KERN_DEBUG "exiting ...\n");
	del_timer(&zc_timer);
	printk(KERN_DEBUG "timer del ...\n");
	netlink_kernel_release(sk);
}

static void nl_custom_data_ready(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	void *payload;
	struct sk_buff *out_skb;
	void *out_payload;
	struct nlmsghdr *out_nlh;
	int payload_len;
	nlh = nlmsg_hdr(skb);
	switch(nlh->nlmsg_type){
		case NLMSG_SETECHO:
			break;
		case NLMSG_GETECHO:
			payload = nlmsg_data(nlh);
			payload_len = nlmsg_len(nlh);
			printk(KERN_DEBUG  "payload_lens= %d\n", payload_len);
			printk(KERN_DEBUG  "Received: %s, From: %d\n", (char *)payload, nlh->nlmsg_pid);
			out_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);//分配足以存放默认大小的sk_buff
			if (!out_skb) goto failure;
			out_nlh = nlmsg_put(out_skb, 0, 0, NLMSG_SETECHO, payload_len, 0);
			if (!out_nlh) goto failure;
			out_payload = nlmsg_data(out_nlh);
			strcpy(out_payload, "[From kernel]:");
			strcat(out_payload, payload);
			nlmsg_unicast(sk, out_skb, nlh->nlmsg_pid);
		default:
			printk(KERN_DEBUG  "Unknow msgtype Received!\n");

	}
	return;
failure:
	printk(KERN_DEBUG  "failed in fun dataready\n");
}

module_init(nl_custom_init);
module_exit(nl_custom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zc");