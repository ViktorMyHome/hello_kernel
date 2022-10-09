#include <linux/module.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/string.h>
#include "netLinkKernelUtils.h"

#define NETLINK_TEST_PROTOCOL 30

static struct sock *nl_sk = NULL;

static void netlink_recv_msg_fn(struct sk_buff *skb_in){

	struct nlmsghdr *nlh_recv, *nlh_reply;
//*nlh_recv - point on data which will be received
	int user_space_process_port_id;
	char *user_space_data; //payload
	int user_data_len;
	int user_space_data_len;
//memory to store reply message from kernel to USA
	char kernel_reply[256];
	struct sk_buff *skb_out;

	printk(KERN_INFO "%s() function invoked", __FUNCTION__);
	nlh_recv = (struct nlmsghdr*)(skb_in->data);

//---------------------------------
	nlmsg_dump_viktor(nlh_recv);
//---------------------------------


	user_space_process_port_id = nlh_recv->nlmsg_pid;
//nlmsg_pid - member of nlm header
	printk(KERN_INFO "%s(%d) : port id of the sending user space process = %u\n",__FUNCTION__, __LINE__, user_space_process_port_id);
	user_space_data = (char*)nlmsg_data(nlh_recv);//payload sent from user space to kernel space
	user_space_data_len = skb_in->len;
	printk(KERN_INFO "%s(%d) : msg recvd from user space=%s, skb_in->len=%d, nlh->nlmsg_len=%d\n",
	__FUNCTION__, __LINE__, user_space_data, user_space_data_len, nlh_recv->nlmsg_len);

//kernel reply onlllly if NLM_F_ACK flag is set in flag field in nl header
	if(nlh_recv->nlmsg_flags & NLM_F_ACK){
		memset(kernel_reply, 0, sizeof(kernel_reply));
		printk(KERN_INFO "nlh_recv->nlmsg_pid=%d\n", nlh_recv->nlmsg_pid);
//		sprintf(kernel_reply, sizeof(kernel_reply), "Msg from Process %d has been processed by kernel", nlh_recv->nlmsg_pid);
		sprintf(kernel_reply, "Msg from Process %d has been processed by kernel", nlh_recv->nlmsg_pid);
		skb_out = nlmsg_new(sizeof(kernel_reply), 0);
		printk(KERN_INFO "After nlmsg_new\n");
		nlh_reply = nlmsg_put(skb_out,
			0,
			nlh_recv->nlmsg_seq,//sequence number
			NLMSG_DONE,//nlm type
			sizeof(kernel_reply),//sizeof data
			0);//flag field

		strncpy(nlmsg_data(nlh_reply), kernel_reply, sizeof(kernel_reply));
		int res = nlmsg_unicast(nl_sk, skb_out, user_space_process_port_id);
		if(res<0){
			printk(KERN_INFO "Error while sending the data back to user-space\n");
			kfree_skb(skb_out);
		}
                 //in case if res>0, message sent, skb_out will be released automatically
	}
}

static struct netlink_kernel_cfg cfg={

	.input = netlink_recv_msg_fn,
};


static int __init helloNetlink_init(void){

	printk(KERN_INFO "Hello Kernel, I am kernel Module helloNetlinkLKM.ko\n");
	nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST_PROTOCOL, &cfg);
//in cfg function executed when get message from kernel
	if(!nl_sk){
		printk(KERN_INFO "Kernel Netlink socket for Netlink protocol %u failed.\n", NETLINK_TEST_PROTOCOL);
		return -ENOMEM;
	}
	printk(KERN_INFO "Netlink Socket Created Suvvessfully");
	//init_net-global  variable, represents complete networking subsystem running in kernel space
	return 0;
}

static void __exit helloNetlink_exit(void){

	printk(KERN_INFO "Bye. Exiting kernel Module helloNetlinkLKM.ko\n");
	netlink_kernel_release(nl_sk);
	nl_sk=NULL;
}

module_init(helloNetlink_init);
module_exit(helloNetlink_exit);

MODULE_LICENSE("GPL");
