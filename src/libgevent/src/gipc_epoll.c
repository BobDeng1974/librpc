/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    epoll.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-04-27 00:59
 * updated: 2015-07-12 00:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/epoll.h>
#include <fcntl.h>


#include "gipc_event.h"


//http://www.cnblogs.com/Anker/archive/2013/08/17/3263780.html
/* http://blog.csdn.net/yusiguyuan/article/details/15027821
   EPOLLET:
   ��EPOLL��Ϊ��Ե����(Edge Triggered)ģʽ,
   ���������ˮƽ����(Level Triggered)��˵��

   EPOLL�¼�������ģ�ͣ�
   Edge Triggered(ET)		
   ���ٹ�����ʽ,�����ʱȽϴ�,ֻ֧��no_block socket (������socket)
   LevelTriggered(LT)		
   ȱʡ������ʽ����Ĭ�ϵĹ�����ʽ,֧��blocksocket��no_blocksocket,�����ʱȽ�С.

   EPOLLIN:
   listen fd,������������,�Զ˷�����ͨ����

   EPOLLPRI:
   �н��������ݿɶ�(����Ӧ�ñ�ʾ�д������ݵ���)

   EPOLLERR:
   ��ʾ��Ӧ���ļ���������������

   EPOLLHUP:
   ��ʾ��Ӧ���ļ����������Ҷ� 
   �Զ������ر�(������close(),shell��kill��ctr+c),����EPOLLIN��EPOLLRDHUP,
   ���ǲ�����EPOLLERR ��EPOLLHUP.��man epoll_ctl���º������¼���˵��,
   ������Ӧ���Ǳ��ˣ�server�ˣ�����Ŵ�����.

   EPOLLRDHUP:
   ���������Щϵͳ��ⲻ��,����ʹ��EPOLLIN,read����0,ɾ�����¼�,�ر�close(fd)

   EPOLLONESHOT:
   ֻ����һ���¼�,������������¼�֮��,
   �������Ҫ�����������socket�Ļ�,
   ��Ҫ�ٴΰ����socket���뵽EPOLL������

   �Զ��쳣�Ͽ�����(ֻ���˰�����),û�����κ��¼���

   epoll���ŵ㣺
	1.֧��һ�����̴򿪴���Ŀ��socket������(FD)
	select ������ܵ���һ���������򿪵�FD����һ�����Ƶģ�
	��FD_SETSIZE���ã�Ĭ��ֵ��2048��
	������Щ��Ҫ֧�ֵ�����������Ŀ��IM��������˵��Ȼ̫���ˡ�
	��ʱ����һ�ǿ���ѡ���޸������Ȼ�����±����ںˣ�
	��������Ҳͬʱָ���������������Ч�ʵ��½���
	���ǿ���ѡ�����̵Ľ������(��ͳ�� Apache����)��
	������Ȼlinux���洴�����̵Ĵ��۱Ƚ�С�����Ծ��ǲ��ɺ��ӵģ�
	���Ͻ��̼�����ͬ��Զ�Ȳ����̼߳�ͬ���ĸ�Ч������Ҳ����һ�������ķ�����
	���� epoll��û��������ƣ�����֧�ֵ�FD�����������Դ��ļ�����Ŀ��
	�������һ��Զ����2048,�ٸ�����,��1GB�ڴ�Ļ����ϴ�Լ��10�����ң�
	������Ŀ����cat /proc/sys/fs/file-max�쿴,һ����˵�����Ŀ��ϵͳ�ڴ��ϵ�ܴ�
	
	2.IOЧ�ʲ���FD��Ŀ���Ӷ������½�
	��ͳ��select/poll��һ������������ǵ���ӵ��һ���ܴ��socket���ϣ�
	��������������ʱ����һʱ��ֻ�в��ֵ�socket��"��Ծ"�ģ�
	����select/pollÿ�ε��ö�������ɨ��ȫ���ļ��ϣ�����Ч�ʳ��������½���
	����epoll������������⣬��ֻ���"��Ծ"��socket���в���---
	������Ϊ���ں�ʵ����epoll�Ǹ���ÿ��fd�����callback����ʵ�ֵġ�
	��ô��ֻ��"��Ծ"��socket�Ż�������ȥ���� callback������
	����idle״̬socket�򲻻ᣬ������ϣ�epollʵ����һ��"α"AIO��
	��Ϊ��ʱ���ƶ�����os�ںˡ���һЩ benchmark�У�
	������е�socket�����϶��ǻ�Ծ��---����һ������LAN������
	epoll������select/poll��ʲôЧ�ʣ��෴���������ʹ��epoll_ctl,
	Ч����Ȼ�����΢���½�������һ��ʹ��idle connectionsģ��WAN����,
	epoll��Ч�ʾ�Զ��select/poll֮����

	3.ʹ��mmap�����ں����û��ռ����Ϣ����
	���ʵ�����漰��epoll�ľ���ʵ���ˡ�
	������select,poll����epoll����Ҫ�ں˰�FD��Ϣ֪ͨ���û��ռ䣬
	��α��ⲻ��Ҫ���ڴ濽���ͺ���Ҫ��������ϣ�
	epoll��ͨ���ں����û��ռ�mmapͬһ���ڴ�ʵ�ֵġ�
	�����������һ����2.5�ں˾͹�עepoll�Ļ���һ�����������ֹ� mmap��һ���ġ�

	4.�ں�΢��
	��һ����ʵ����epoll���ŵ��ˣ���������linuxƽ̨���ŵ㡣
	Ҳ������Ի���linuxƽ̨���������޷��ر�linuxƽ̨������΢���ں˵�������
	���磬�ں�TCP/IPЭ��ջʹ���ڴ�ع���sk_buff�ṹ��
	��ô����������ʱ�ڶ�̬��������ڴ�pool(skb_head_pool)�Ĵ�С--- 
	ͨ��echoXXXX>/proc/sys/net/core/hot_list_length��ɡ�
	�ٱ���listen�����ĵ�2������(TCP���3�����ֵ����ݰ����г���)��
	Ҳ���Ը�����ƽ̨�ڴ��С��̬������
	��������һ�����ݰ�����Ŀ�޴�ͬʱÿ�����ݰ������Сȴ��С������ϵͳ��
	�������µ�NAPI���������ܹ���

	
  */

/* ����ע���¼�, �޸��¼�, ɾ���¼� 
   EPOLL_CTL_ADD EPOLL_CTL_MOD EPOLL_CTL_DEL */ 


#ifdef __ANDROID__
#define EPOLLRDHUP      (0x2000)
#endif

#define EPOLL_MAX_NEVENT    (4096)
#define MAX_SECONDS_IN_MSEC_LONG \
        (((LONG_MAX) - 999) / 1000)

#define LISTEN_EPOLLEVENTS 	10
#define LISTEN_EPOLLFDSIZE 	10


struct epoll_ctx {
    int epfd;
    int nevents;
 	struct epoll_event epev;
    struct epoll_event *events;
};

static void *epoll_init(void)
{
    int fd = -1;
    struct epoll_ctx *ec;
    fd = epoll_create(LISTEN_EPOLLFDSIZE);
    if (-1 == fd) {
        printf("errno = %d %s\n", errno, strerror(errno));
        return NULL;
    }
    ec = (struct epoll_ctx *)malloc(sizeof(struct epoll_ctx));
    if (!ec) {
        printf("malloc epoll_ctx failed!\n");
        return NULL;
    }
	memset(ec, 0, sizeof(struct epoll_ctx));
    ec->epfd = fd;
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	
    ec->nevents = LISTEN_EPOLLEVENTS;
    ec->events = (struct epoll_event *)malloc(sizeof(struct epoll_event)*LISTEN_EPOLLEVENTS);
    if (!ec->events) {
        printf("malloc epoll_event failed!\n");
        return NULL;
    }
	memset(ec->events, 0, sizeof(struct epoll_event)*LISTEN_EPOLLEVENTS);
    return ec;
}

static void epoll_deinit(void *ctx)
{
    struct epoll_ctx *ec = (struct epoll_ctx *)ctx;
    if (!ctx) {
        return;
    }
	close(ec->epfd);
    free(ec->events);
    free(ec);
	/*
	���������ͬ��select()�еĵ�һ��������������������fd+1��ֵ��
	��Ҫע����ǣ���������epoll����������ǻ�ռ��һ��fdֵ��
	��linux������鿴/proc/����id/fd/�����ܹ��������fd�ģ�
	������ʹ����epoll�󣬱������close()�رգ�������ܵ���fd���ľ���*/
}

static int epoll_add(struct gevent_base *eb, struct gevent *e)
{
    struct epoll_ctx *ec = (struct epoll_ctx *)eb->ctx;

    if (e->flags & EVENT_READ)
        ec->epev.events |= EPOLLIN;
    if (e->flags & EVENT_WRITE)
        ec->epev.events |= EPOLLOUT;
    if (e->flags & EVENT_ERROR)
        ec->epev.events |= EPOLLERR;
    //ec->epev.events |= EPOLLET;
	//ec->epev.events |= EPOLLRDHUP;
	//ec->epev.events |= EPOLLHUP;
    ec->epev.data.ptr = (void *)e;

    if (-1 == epoll_ctl(ec->epfd, EPOLL_CTL_ADD, e->evfd, &ec->epev)) {
        printf("errno = %d %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

static int epoll_mod(struct gevent_base *eb, struct gevent *e)
{
    return 0;
}


static int epoll_del(struct gevent_base *eb, struct gevent *e)
{
    struct epoll_ctx *ec = (struct epoll_ctx *)eb->ctx;
    if (-1 == epoll_ctl(ec->epfd, EPOLL_CTL_DEL, e->evfd, NULL)) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

static int epoll_dispatch(struct gevent_base *eb, struct timeval *tv)
{
    struct epoll_ctx *epop = (struct epoll_ctx *)eb->ctx;
    struct epoll_event *events = epop->events;
    int i, n;
    int timeout = -1;

    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }
    n = epoll_wait(epop->epfd, events, epop->nevents, timeout);
    if (-1 == n) {
        if (errno != EINTR) {
            printf("epoll_wait failed %d: %s\n", errno, strerror(errno));
            return -1;
        }
        return 0;
    }
    if (0 == n) {
        printf("epoll_wait timeout\n");
        return 0;
    }
    for (i = 0; i < n; i++) {
        int what = events[i].events;
        struct gevent *e = (struct gevent *)events[i].data.ptr;

        if (what & (EPOLLHUP | EPOLLERR)) {
			 e->evcb->ev_err(e->evfd, (void *)e->evcb->args);
        } else {
            if (what & EPOLLIN) {
                e->evcb->ev_in(e->evfd, (void *)e->evcb->args);
				//events[i].events= EPOLLOUT | EPOLLET;
				events[i].events = EPOLLIN;
				epoll_ctl(epop->epfd, EPOLL_CTL_MOD, e->evfd, &epop->epev);
			
        	}
            if (what & EPOLLOUT) {
                e->evcb->ev_out(e->evfd, (void *)e->evcb->args);
				events[i].events = EPOLLIN;
				epoll_ctl(epop->epfd, EPOLL_CTL_MOD, e->evfd, &epop->epev);
        	}
            if (what & EPOLLRDHUP)
                e->evcb->ev_err(e->evfd, (void *)e->evcb->args);
        }
    }
    return 0;
}

struct gevent_ops epollops = {
    .init     = epoll_init,
    .deinit   = epoll_deinit,
    .add      = epoll_add,
    .del      = epoll_del,
    .dispatch = epoll_dispatch,
};
