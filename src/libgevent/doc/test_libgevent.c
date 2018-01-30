/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libgevent.c
 * author:  gozfree <gozfree@163.com>
 * created: 2016-04-10 12:51
 * updated: 2016-04-10 12:51
 ******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#include "libgevent.h"


static void read_cb(int fd, void *arg)
{
    printf("input %d.......\n", fd);
}

static int foo(void)
{
    int fd = 0;
    struct gevent_base *evbase = gevent_base_create();
    if (!evbase) {
        printf("gevent_base_create failed!\n");
        return -1;
    }
    struct gevent *event = gevent_create(fd, on_input, NULL, NULL, NULL);
    if (!event) {
        printf("gevent_create failed!\n");
        return -1;
    }
    if (-1 == gevent_add(evbase, event)) {
        printf("gevent_add failed!\n");
        return -1;
    }
    gevent_base_loop(evbase);

	
    gevent_del(evbase, event);
    gevent_base_destroy(evbase);

    return 0;
}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
