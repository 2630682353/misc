#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

MODULE_LICENSE("GPL");

/* THESE values are used to keep the USERname and PASSword until
 * they are queried. Only one USER/PASS pair will be held at one
 * time and will be cleared once queried. */

/* Used to describe our Netfilter hooks */
struct nf_hook_ops  pre_hook;          /* Incoming */
struct nf_hook_ops  post_hook;         /* Outgoing */


/* Function that looks at an sk_buff that is known to be an FTP packet.
 * Looks for the USER and PASS fields and makes sure they both come from
 * the one host as indicated in the target_xxx fields */
static void check_http(struct sk_buff *skb)
{
   struct tcphdr *tcp;
   char *host;
   char *_and;
   char *str_host;
   char *data;
   int len;

//   printk("1111111111111111111111111111111111111111111111111111111111");

   tcp = tcp_hdr(skb);
	if (tcp->rst == 1) {
		printk("syn packet\n");
		return ;
	}
   data = (char *)((unsigned long)tcp + (unsigned long)(tcp->doff * 4));


 	if (strstr(data, "Host: ") != NULL && strstr(data, "Connection") != NULL) {
		host = strstr(data, "Host: ");
		_and = strstr(host, "\r\n");
		len = _and - host;
		if ((str_host = kmalloc(len + 1, GFP_KERNEL)) == NULL)
			return;
		memcpy(str_host, host, len);
		str_host[len] = '\0';
		printk("%s\n", str_host);
		kfree(str_host);

 	} else {

//      printk("44444444444444444444444444444444444444444444444444444444444444");
      return;
   }

}

/* Function called as the POST_ROUTING (last) hook. It will check for
 * FTP traffic then search that traffic for USER and PASS commands. */
static unsigned int watch_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in,
					const struct net_device *out, int (*okfn)(struct sk_buff *))
{
   struct sk_buff *sb = skb;
   struct tcphdr *tcp;
   //printk("666666666666666666666666666666666666666666666666666666666666666666666");
   /* Make sure this is a TCP packet first */
   if (ip_hdr(sb)->protocol != IPPROTO_TCP)
     return NF_ACCEPT;             /* Nope, not TCP */

   tcp = (struct tcphdr *)((sb->data) + (ip_hdr(sb)->ihl * 4));

   /* Now check to see if it's an FTP packet */
   if (tcp->dest != htons(80))
     return NF_ACCEPT;             /* Nope, not FTP */

   /* Parse the FTP packet for relevant information if we don't already
    * have a username and password pair. */
//   if (!have_pair)
   check_http(sb);

   /* We are finished with the packet, let it go on its way */
   return NF_ACCEPT;
}


int init_module()
{
 
   post_hook.hook     = watch_out;
   post_hook.pf       = PF_INET;
   post_hook.priority = NF_IP_PRI_FIRST;
   post_hook.hooknum  = NF_INET_FORWARD;

   nf_register_hook(&post_hook);

   printk("88888888888888888888888888888888888888888888888888888888888888888\n");

   return 0;
}

void cleanup_module()
{
   nf_unregister_hook(&post_hook);
   printk("99999999999999999999999999999999999999\n");
}