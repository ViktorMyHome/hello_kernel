#include <linux/socket.h>
#include <linux/netlink.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>
#include <pthread.h>
//#include "netLinkKernelUtils.h"

#define NLMSG_GREET 20
#define NETLINK_TEST_PROTOCOL 30
#define MAX_PAYLOAD 256


int send_netlink_msg_to_kernel(int sock_fd,
				char *msg,
				uint32_t msg_size,
				int nlmsg_type,
				uint16_t flags)
{
	struct sockaddr_nl dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;

	struct nlmsghdr *nlh=(struct nlmsghdr *)calloc(1, NLMSG_HDRLEN + NLMSG_SPACE(MAX_PAYLOAD));

	nlh->nlmsg_len = NLMSG_HDRLEN + NLMSG_SPACE(msg_size);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = flags;
	nlh->nlmsg_seq = 0;

	strncpy(NLMSG_DATA(nlh), msg, msg_size);

	struct iovec iov;
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;

	static struct msghdr outermsghdr;

	memset(&outermsghdr, 0, sizeof(struct msghdr));
	outermsghdr.msg_name = (void*)&dest_addr;
	outermsghdr.msg_namelen = sizeof(dest_addr);
	outermsghdr.msg_iov = &iov;
	outermsghdr.msg_iovlen =1;
	int rc = sendmsg(sock_fd, &outermsghdr, 0);
	if(rc<0){
		printf("Msg Sending Failed, error no=%d\n", errno);
	}//errno -global variable
	return rc;
}

static void
greet_kernel(int sock_fd, char *msg, uint32_t msg_len){
	send_netlink_msg_to_kernel(sock_fd, msg, msg_len, NLMSG_GREET, NLM_F_ACK);
}



int
create_netlink_socket(int protocol_number){

     /* Create a net link socket using usual socket() system call
      * When SOCK_RAW is used, Application has to pepare the struct msghdr
      * structure and send msg using sendmsg().
      * When SOCK_DGRAM is used, socket layer will take care to prepare struct
      * msghdr for you. You have to use sendto() in this case.
      * In this file, I have demonstrated SOCK_RAW case
      * */

    int sock_fd = socket(PF_NETLINK, 
                         SOCK_RAW, 
                         protocol_number);

    return sock_fd;
}


static void
exit_userspace(int sock_fd){
    close(sock_fd);
}

typedef struct thread_arg_{
	int sock_fd;
}thread_arg_t;

static void *_start_kernel_data_receive_thread(void *arg){
	int rc=0;
	struct iovec iov;
	struct nlmsghdr *nlh_recv=NULL;
	static struct msghdr outermsghdr;
	int sock_fd=0;

	thread_arg_t *thread_arg=(thread_arg_t*)arg;
	sock_fd = thread_arg->sock_fd;

	//buffer to recv data from kernel
	nlh_recv=(struct nlmsghdr *)calloc(1, NLMSG_HDRLEN + NLMSG_SPACE(MAX_PAYLOAD));

	do{
		memset(nlh_recv, 0, NLMSG_HDRLEN+NLMSG_SPACE(MAX_PAYLOAD));
		iov.iov_base = (void *)nlh_recv;
		iov.iov_len = NLMSG_HDRLEN+NLMSG_SPACE(MAX_PAYLOAD);
		
		memset(&outermsghdr, 0, sizeof(struct msghdr));

		outermsghdr.msg_iov = &iov;
		outermsghdr.msg_name = NULL;
		outermsghdr.msg_iovlen = 1;
		outermsghdr.msg_namelen = 0;

		rc = recvmsg(sock_fd, &outermsghdr, 0);

		nlh_recv = outermsghdr.msg_iov->iov_base;
		char *payload = NLMSG_DATA(nlh_recv);
		printf("Recceived Netlink msg from kernel, bytes recvd=%d\n",rc);
		printf("msg recvdd from kernel = %s\n", payload);

	}while(1);
}

void start_kernel_data_receive_thread(thread_arg_t *thread_arg){

	pthread_attr_t attr;
	pthread_t recv_pkt_thread;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&recv_pkt_thread, &attr, 
		_start_kernel_data_receive_thread, (void *)thread_arg);
}

int main(int argc, char **argv){

	int choice;
	int sock_fd;
	sock_fd = create_netlink_socket(NETLINK_TEST_PROTOCOL);

	if(sock_fd == -1){
		printf("Error: Netlink socket creation failed error = %d\n", errno);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_nl src_addr;
	struct nlmsghdr *nlh = NULL;
	memset(&src_addr, 0, sizeof(src_addr));

	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();

	if(bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr))==-1){
		printf("Error: Bind has failed\n");
		exit(1);
	}

	thread_arg_t thread_arg;
	thread_arg.sock_fd = sock_fd;

	start_kernel_data_receive_thread(&thread_arg);

	while(1)
	{
		printf("Main-Menu\n");
		printf("\t1. Greet Kernel\n");
		printf("\t2. Exit\n");
		printf("choice ");
		scanf("%d\n", &choice);
		switch(choice){
		 case 1:
			{
			char user_msg[MAX_PAYLOAD];
			memset(user_msg, 0, MAX_PAYLOAD);
			if((fgets((char *)user_msg, MAX_PAYLOAD, stdin)==NULL)){
				printf("error in reading from stdin\n");
				exit(EXIT_FAILURE);
			}
				greet_kernel(sock_fd, user_msg, strlen(user_msg));
			}
			break;
		 case 2:
			exit_userspace(sock_fd);
			break;
		default:
			break;
			
		}
	}

	return 0;
}

