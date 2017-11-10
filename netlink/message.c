#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include "memory_pool.h"
#include "thread_pool.h"
#include "queue.h"
#include "type.h"
#include "debug.h"
#include <errno.h>
#include "sock.h"


#define BUF_POOL_UNITSIZE 8192
#define BUF_POOL_INITNUM 10
#define BUF_POOL_GROWNUM 8
#define ITEM_POOL_UNITSIZE sizeof(queue_item_t)
#define ITEM_POOL_INITNUM 5
#define ITEM_POOL_GROWNUM 4
#define PKT_POOL_UNITSIZE sizeof(msg_pkt_t)
#define PKT_POOL_INITNUM 5
#define PKT_POOL_GROWNUM 4
#define RECV_FROM_TIMEOUT 3

#define NETLINK_EVENT 30
#define NETLINK_UMSG 31

static int16 s_module_id = 0;

/*数据包信息结构体,用在queue_item_t的arg中传递收/发的数据包*/
typedef struct msg_pkt_st{
    sock_addr_u paddr;  /*peer address information*/
    int16       dmid;   /*destination module id, equal to peer module id*/
    int16       smid;   /*source module id, equal to self module id*/
    buffer_t    in;     /*received packet buffer*/
    buffer_t    out;    /*send packet buffer*/
}msg_pkt_t;

/*typedef struct msg_pkt_queue_st{
    queue_head_t    queue;
    pthread_mutex_t mutex;
}msg_pkt_queue_t;
*/
typedef struct msg_dst_cmd{
	msg_cmd_e cmd;
	struct list_head cmd_list;	/*对端模块的命令列表*/
}msg_dst_cmd_t;



typedef struct msg_module_st{
	struct list_head list;		/*已注册对端模块的链表*/
	int32 mid;					/*对端模块ID*/
	sock_addr_u addr;
	struct list_head cmd_list;	/*对端模块的命令列表*/
}msg_module_t;

typedef struct msg_module_cmd_st{
	struct list_head list;	/*已注册命令的链表*/
	int32 cmd;				/*命令*/
	msg_cmd_handle_cb func; /*命令的回调函数*/
}msg_module_cmd_t;



static thread_pool_t *sp_handle_pool = NULL;    /*业务处理线程池*/



static mempool_t *mempool_buf = NULL;  /*数据内存池*/
static mempool_t *mempool_queue_item = NULL;  /*队列元素内存池*/
static mempool_t *mempool_pkt = NULL;  /*数据包内存池*/

static pthread_cond_t snd_thread_cond; /*发送线程条件变量*/

static socket_t *sp_nlk_rcv_sock = NULL;
static socket_t *sp_nlk_snd_sock = NULL;




//static queue_head_t *sp_idle_queue = NULL;  /*闲包队列,队列中存储的都是待接收数据的包*/

static pthread_t pt_snd = 0;
static pthread_t pt_rcv = 0;

static LIST_HEAD(s_dst_modules);    /*通信对端模块信息*/
static LIST_HEAD(s_self_cmds);    /*本模块注册的命令*/


static void *msg_rcv_thread_cb(void *arg);
static void *msg_snd_thread_cb(void *arg);
static msg_module_cmd_t *msg_get_cmdfunc(const int32 cmd);




int32 msg_init(const int16 module_id)   /*本模块的模块ID*/
         		
{
    	
	
}




void msg_final(void)
{
  
	msg_dst_module_unregister(-1);
	msg_cmd_unregister_all();
	
}

/*请求消息发送接口*/
int32 msg_send_syn(int16 dmid,  /*对端模块ID*/
                   int32 cmd,   /*命令*/
                   void *sbuf,  /*发包buffer*/
                   int32 slen,  /*sbuf中数据长度*/
                   void **rbuf, /*收包buffer*/
                   int32 *rlen) /*入参为rbuf的存储空间总大小,返回rbuf中实际收到的数据的长度*/
{
    msg_module_t* p_module = NULL;
	msg_dst_cmd_t* p_cmd = NULL;
	sock_addr_u *addr = NULL;
	list_for_each_entry(p_module, &s_dst_modules, list) {
		if ( p_module->mid == dmid ) {
			list_for_each_entry(p_cmd, &p_module->cmd_list, cmd_list) {
				if (p_cmd->cmd == cmd) {
					addr = &p_module->addr;
					break;
				}
			}
			break;
		}
	}
	if (!addr) {
		DB_ERR("dst addr null");
		goto out;
	}

	
	int len = 0;
	len = send_to_user(cmd, sbuf, slen, addr->nl_addr.nl_groups);
		
	return len;
	
}

/*业务处理函数,本接口是在业务线程回调函数中调用的*/
static void msg_handle(queue_item_t *item)
{
    /*1. 数据有效性和合法性检查*/
    /*2. 将pkt->in.buf中的数据强制转换为msg消息*/
    /*3. 判定消息的有效性和合法性*/
    /*4. 根据msg->cmd的不同调用不同的命令的处理函数进行处理
     *  4.1. 如果msg->cmd不能够本模块处理不了,则返回对应的不支持该功能的错误消息
     */
    /*5. 根据命令处理的结果封装应答消息*/
    /*6. 将应答消息加入到pkt->out成员中*/
    /*7. 获取发包队列的锁*/
    /*8. 将pkt入队到发包队列中*/
    /*9. 释放发包队列的锁*/

	msg_pkt_t *pkt = (msg_pkt_t*)(item->arg);
	if (NULL == pkt)
		return;
	msg_t *recv_msg = (msg_t*)(pkt->in.buf + pkt->in.offset);
	int32 cmd = recv_msg->cmd;
	msg_module_cmd_t *msg_cmd = msg_get_cmdfunc(cmd);
	if (!msg_cmd)
		DB_ERR("cmd not registered");
	
	pkt->out.buf = mem_alloc(mempool_buf);
	
	pkt->out.size = mempool_buf->unitsize;
	int olen = mempool_buf->unitsize - sizeof(msg_t);
	pkt->out.offset = pkt->in.offset;
	msg_t *snd_msg = ((msg_t*)(pkt->out.buf + pkt->out.offset));
	memset(recv_msg->data + recv_msg->dlen, 0, 1);
	
	DB_INF("before func dlen:%s    %d  thread_id:%ld",recv_msg->data, recv_msg->dlen, pthread_self());
	snd_msg->result = msg_cmd->func(cmd, recv_msg->data, recv_msg->dlen, snd_msg->data, &olen);
	DB_INF("after func");
	snd_msg->dlen = olen;
	snd_msg->cmd = recv_msg->cmd;
	snd_msg->dmid = recv_msg->smid;
	
	pkt->out.len = pkt->out.offset + sizeof(msg_t) + olen;
	
	if (pkt->in.offset == sizeof(struct nlmsghdr)) {
		
		struct nlmsghdr *nlh = (struct nlmsghdr *)pkt->out.buf;
		nlh->nlmsg_len = pkt->out.len;
        nlh->nlmsg_flags = 0;
        nlh->nlmsg_type = 0;
        nlh->nlmsg_seq = 0;
        nlh->nlmsg_pid = (int)getpid(); //self port
	}
	
	pthread_mutex_lock(&sp_snd_queue->mutex);
	queue_enqueue(sp_snd_queue, item);
	pthread_mutex_unlock(&sp_snd_queue->mutex);
	pthread_cond_signal(&snd_thread_cond);
	
	DB_INF("after msg_handle");
	
}

/*业务线程回调函数*/
static void msg_handle_cb(void *arg)
{
    /*1. 获取收包队列的锁*/
    /*2. 从收包队列中出队第一个msg_pkt_t数据包pkt*/
    /*3. 释放收包队列的锁*/
    /*4. 调用msg_handle()函数,并传入pkt进行业务处理*/
	queue_item_t *item = NULL;
	pthread_mutex_lock(&(sp_rcv_queue->mutex));
	if (FALSE ==  queue_empty(sp_rcv_queue))
		item = queue_dequeue(sp_rcv_queue);
	else {
		pthread_mutex_unlock(&(sp_rcv_queue->mutex));
		return;
	}
	pthread_mutex_unlock(&(sp_rcv_queue->mutex));
	msg_handle(item);
	
	
}

/*发包线程回调函数*/
static void *msg_snd_thread_cb(void *arg)
{
    while (1)
    {
        /*1. 获取发包队列的锁*/
        /*2. 查看发包队列是否为空的
         *  2.1. 如果是空的,则释放发包队列锁,并sleep(1),随后继续从第1步开始轮训;
         *  2.2. 如果不为空,则继续执行第3步*/
        /*3. 从发包队列中出队一个msg_pkt_t的数据包pkt*/
        /*4. 释放发包队列的锁*/
        /*5. 通过socket发送数据包,数据包发送的目的地址信息在pkt的paddr成员中存储着*/
        /*6. 如果线程正在销毁,则退出while(1)的循环*/

		pthread_mutex_lock(&(sp_snd_queue->mutex));
		if (TRUE == queue_empty(sp_snd_queue)) {
			pthread_cond_wait(&snd_thread_cond, &(sp_snd_queue->mutex));
		}

		queue_item_t *item = queue_dequeue(sp_snd_queue);
		pthread_mutex_unlock(&(sp_snd_queue->mutex));
		msg_pkt_t *snd_pkt = (msg_pkt_t*)(item->arg);
		
		DB_INF("server before sock_sendto: %d", snd_pkt->out.len);
		if (snd_pkt->out.offset == sizeof(struct nlmsghdr))
			sock_sendto(sp_nlk_snd_sock, snd_pkt->out.buf, snd_pkt->out.len, &snd_pkt->paddr);
		else
			sock_sendto(sp_snd_sock, snd_pkt->out.buf, snd_pkt->out.len, &snd_pkt->paddr);
		DB_INF("server have send");
		
		mem_free(snd_pkt->in.buf);
		mem_free(snd_pkt->out.buf);
		mem_free(item->arg);
		mem_free(item);
		
    }
}

/*收包线程回调函数*/
static void *msg_rcv_thread_cb(void *arg)
{
	fd_set fds;
	int max_fd = 0, ipc_fd;
	struct timeval tv;
    while (1)
    {
        /*1. 通过select()函数在sp_rcv_sock上进行监听收包,超时时间为2秒
         *  1.1. 如果超时且没有数据包可以收取,则sleep(1),随后继续从第1步开始轮训;
         *  1.2. 如果有数据包待收取,则继续执行后续的步骤*/
        /*2. 分配相应的收包buffer*/
        /*3. 调用sock_recvfrom()函数从sp_rcv_sock上收取数据包,并将对端地址信息一并获取回来,以便发送响应报文时使用*/
        /*4. 分配一个msg_pkt_t结构的对象pkt,并将收取的数据包buffer添加到pkt->in中,且将对端地址信息存入到pkt->paddr中*/
        /*5. 获取收包队列的锁*/
        /*6. 将pkt入队到收包队列中*/
        /*7. 释放收包队列的锁*/
        /*8. 调用thread_pool_worker_add()函数往业务线程池中增加一个worker*/
        //thread_pool_worker_add(sp_handle_pool, msg_handle_cb, NULL);
        /*9. 如果收包线程正在销毁,则推出while(1)循环*/
		tv.tv_sec = 60;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(sp_rcv_sock->fd, &fds);
		if (sp_rcv_sock->fd > max_fd)
			max_fd = sp_rcv_sock->fd;
		
		FD_SET(sp_nlk_rcv_sock->fd, &fds);
		if (sp_nlk_rcv_sock->fd > max_fd)
			max_fd = sp_nlk_rcv_sock->fd;
		
		if (select(max_fd + 1, &fds, NULL, NULL, &tv) < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
		}
		if (FD_ISSET(sp_rcv_sock->fd, &fds)) {
			msg_pkt_t* msg = (msg_pkt_t*)mem_alloc(mempool_pkt);
			msg->in.buf = mem_alloc(mempool_buf);
			msg->in.offset = 0;
			msg->in.size = mempool_buf->unitsize;
			if (sock_recvfrom(sp_rcv_sock, msg->in.buf, msg->in.size, &msg->paddr) < 0) {
				mem_free(msg->in.buf);
				mem_free(msg);
				continue;
			}
			DB_INF("server have recv");
			queue_item_t *item = (queue_item_t *)mem_alloc(mempool_queue_item);
			item->arg = msg;
			pthread_mutex_lock(&sp_rcv_queue->mutex);
			queue_enqueue(sp_rcv_queue, item);
			pthread_mutex_unlock(&sp_rcv_queue->mutex);
			thread_pool_worker_add(sp_handle_pool, msg_handle_cb, NULL);
		}
		if (FD_ISSET(sp_nlk_rcv_sock->fd, &fds)) {
			
			msg_pkt_t* msg = (msg_pkt_t*)mem_alloc(mempool_pkt);
			msg->in.buf = mem_alloc(mempool_buf);
			msg->in.offset = sizeof(struct nlmsghdr);
			msg->in.size = mempool_buf->unitsize;
			if (sock_recvfrom(sp_nlk_rcv_sock, msg->in.buf, msg->in.size, &msg->paddr) < 0) {
				mem_free(msg->in.buf);
				mem_free(msg);
				continue;
			}
			DB_INF("netlink server have recv");

			struct  timeval time;
		    gettimeofday(&time,NULL);
			DB_INF("time is -----------------------------  %d", time.tv_sec);
			
			queue_item_t *item = (queue_item_t *)mem_alloc(mempool_queue_item);
			item->arg = msg;
			pthread_mutex_lock(&sp_rcv_queue->mutex);
			queue_enqueue(sp_rcv_queue, item);
			pthread_mutex_unlock(&sp_rcv_queue->mutex);
			thread_pool_worker_add(sp_handle_pool, msg_handle_cb, NULL);
		}

		
    }
}

/*根据命令获取msg_module_cmd_t*/
static msg_module_cmd_t *msg_get_cmdfunc(const int32 cmd)
{
	msg_module_cmd_t *msg_cmd = NULL;
	list_for_each_entry(msg_cmd, &s_self_cmds, list) {
		if ( msg_cmd->cmd == cmd )
			return msg_cmd;
	}
	return NULL;
}

/*本模块的命令注册*/
int32 msg_cmd_register(const int32 cmd,
                       msg_cmd_handle_cb cb)
{
	if (msg_get_cmdfunc(cmd))
		return -1;
	msg_module_cmd_t *msg_cmd = (msg_module_cmd_t *)malloc(sizeof(msg_module_cmd_t));
	msg_cmd->cmd = cmd;
	msg_cmd->func = cb;
	list_add(&(msg_cmd->list), &s_self_cmds);
	return 0;
}

/*命令注销*/
int32 msg_cmd_unregister(const int32 cmd)
{
	msg_module_cmd_t* p = NULL;
	msg_module_cmd_t* next = NULL;
	list_for_each_entry_safe(p, next, &s_self_cmds, list) {
		if ( p->cmd == cmd ) {
			list_del(&p->list);
			free(p);
			break;
		}
	}
	return 0;
}

/*注销所有命令*/
int32 msg_cmd_unregister_all(void)
{
	msg_module_cmd_t* p = NULL;
	msg_module_cmd_t* next = NULL;
	list_for_each_entry_safe(p, next, &s_self_cmds, list) {	
		list_del(&p->list);
		free(p);
		DB_INF("unregister MY");
	}
	return 0;
}

/*消息通信对端模块信息注册*/
int32 msg_dst_module_register_unix(const int32 mid,          /*对端模块ID*/
								char *path)  /*对端模块地址信息*/
{
	msg_module_t *msg_module = (msg_module_t *)malloc(sizeof(msg_module_t));
	msg_module->mid = mid;
	
	msg_module->addr.un_addr.sun_family = AF_UNIX;
	memset(msg_module->addr.un_addr.sun_path, 0, sizeof(msg_module->addr.un_addr.sun_path));
	strncpy(msg_module->addr.un_addr.sun_path, path, sizeof(msg_module->addr.un_addr.sun_path)-1);
	
	INIT_LIST_HEAD(&msg_module->cmd_list);
	list_add(&(msg_module->list), &s_dst_modules);
	return 0;
}

/*消息通信对端模块信息注册*/
int32 msg_dst_module_register_netlink(const int32 mid, uint32 grp) 		 /*对端模块ID*/
								
{
	msg_module_t *msg_module = (msg_module_t *)malloc(sizeof(msg_module_t));
	msg_module->mid = mid;
	
	msg_module->addr.nl_addr.nl_family = AF_NETLINK;
	msg_module->addr.nl_addr.nl_pid = 0;
	msg_module->addr.nl_addr.nl_groups = grp;
	INIT_LIST_HEAD(&msg_module->cmd_list);
	list_add(&(msg_module->list), &s_dst_modules);
	return 0;
}


/*注销对端模块
	如果为-1注销所有模块
*/

int32 msg_dst_module_unregister(const int32 mid)
{
	msg_module_t* p_module = NULL;
	msg_module_t* next_module = NULL;
	msg_dst_cmd_t* p_cmd = NULL;
	msg_dst_cmd_t* next_cmd = NULL;

	if (mid == -1) {
		list_for_each_entry_safe(p_module, next_module, &s_dst_modules, list) {	
			list_del(&p_module->list);
			list_for_each_entry_safe(p_cmd, next_cmd, &p_module->cmd_list, cmd_list) {
				list_del(&p_cmd->cmd_list);
				free(p_cmd);
				DB_INF("unregister");
			}
			free(p_module);
		}
	} else {
		list_for_each_entry_safe(p_module, next_module, &s_dst_modules, list) {
			if ( p_module->mid == mid ) {
				list_del(&p_module->list);
				list_for_each_entry_safe(p_cmd, next_cmd, &p_module->cmd_list, cmd_list) {
					list_del(&p_cmd->cmd_list);
					free(p_cmd);
				}
				free(p_module);
				break;
			}
		}
	}
	return 0;
}

/*消息通信对端模块接收的命令字添加*/
int32 msg_dst_module_cmd_add(const int32 mid,   /*对端模块ID*/
                             const int32 cmd)   /*对端模块将接收的命令*/
{
	msg_module_t* p_module = NULL;
	msg_dst_cmd_t* p_cmd = NULL;
	list_for_each_entry(p_module, &s_dst_modules, list) {
		if ( p_module->mid == mid ) {
			list_for_each_entry(p_cmd, &p_module->cmd_list, cmd_list) {
				if (p_cmd->cmd == cmd)
					return -1;
			}
			msg_dst_cmd_t* msg_cmd = malloc(sizeof(msg_dst_cmd_t));
			msg_cmd->cmd = cmd;
			list_add(&(msg_cmd->cmd_list), &(p_module->cmd_list));
			return 0;
		}
	}
	return -1;
}

/*消息通信对端模块接收的命令删除
 *如果cmd为-1,则删除所有的命令*/
						
int32 msg_dst_module_cmd_del(const int32 mid,
                             const int32 cmd)
{
	msg_module_t* p_module = NULL;
	msg_module_t* next_module = NULL;
	msg_dst_cmd_t* p_cmd = NULL;
	msg_dst_cmd_t* next_cmd = NULL;
	list_for_each_entry_safe(p_module, next_module, &s_dst_modules, list) {
		if ( p_module->mid == mid ) {
			if (cmd == -1) {
				list_for_each_entry_safe(p_cmd, next_cmd, &p_module->cmd_list, cmd_list) {
					list_del(&p_cmd->cmd_list);
					free(p_cmd);
				}
			}
			else { 
				list_for_each_entry_safe(p_cmd, next_cmd, &p_module->cmd_list, cmd_list) {
					if (p_cmd->cmd == cmd) {
						list_del(&p_cmd->cmd_list);
						free(p_cmd);
					}
				}
			}
			break;
		}
	}
	return 0;
}

/*删除消息通信对端模块接收的所有命令.
 *如果mid为-1,则删除所有的对端模块的所有命令*/
int32 msg_dst_module_cmd_del_all(const int32 mid)
{
	msg_module_t* p_module = NULL;
	msg_module_t* next_module = NULL;
	msg_dst_cmd_t* p_cmd = NULL;
	msg_dst_cmd_t* next_cmd = NULL;
	if (mid == -1) {
		list_for_each_entry_safe(p_module, next_module, &s_dst_modules, list) {
			list_for_each_entry_safe(p_cmd, next_cmd, &p_module->cmd_list, cmd_list) {
					list_del(&p_cmd->cmd_list);
					free(p_cmd);
			}	
		}
	} else {
		list_for_each_entry_safe(p_module, next_module, &s_dst_modules, list) {
			if (p_module->mid == mid) {
				list_for_each_entry_safe(p_cmd, next_cmd, &p_module->cmd_list, cmd_list) {	
					list_del(&p_cmd->cmd_list);
					free(p_cmd);
				}
				break;
			}
		}
	}
	return 0;
}

