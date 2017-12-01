#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/udp.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/inet.h>
#include <linux/netfilter.h>
#include <net/route.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_tables.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("koorey KING");
MODULE_DESCRIPTION("My hook test");

#define NIPQUAD(addr) \
    ((unsigned char *)&addr)[3], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[0]

atomic_t pktcnt = ATOMIC_INIT(0);

static unsigned int myhook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in,
					const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	const struct iphdr *iph = ip_hdr(skb);
	if (iph->protocol == 1) {
		atomic_inc(&pktcnt);
		if (atomic_read(&pktcnt)%5 == 0) {
			printk(KERN_INFO "%d: drop an ICMP pkt to %u.%u.%u.%u!\n", atomic_read(&pktcnt), NIPQUAD(iph->daddr));
			return NF_DROP;
		}
	}
	return NF_ACCEPT;
}

static struct nf_hook_ops nfho = {
	.hook = myhook_func,
	.owner = THIS_MODULE,
	.pf = PF_INET,
	.hooknum = NF_INET_PRE_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

static int __init myhook_init(void)
{
	return nf_register_hook(&nfho);
}

static void __exit myhook_fini(void)
{
	nf_unregister_hook(&nfho);
}

module_init(myhook_init);
module_exit(myhook_fini);