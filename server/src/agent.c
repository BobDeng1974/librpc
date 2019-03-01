/**************************************************************************
*
* Copyright (c) 2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <server.h>

static  __attribute__((constructor(101))) void before1()
{
 
    printf("before1\n");
}
static  __attribute__((constructor(102))) void before2()
{
 
    printf("before2\n");
}


/* �����ʾһ�������ķ���ֵֻ�ɲ�������, �����������Ļ�,
	�Ͳ��ٵ��ô˺�����ֱ�ӷ���ֵ.�����ҵĳ��Է��ֻ��ǵ�����,
	���־������Ϸ���Ҫ��gcc��һ��-O�Ĳ����ſ���.
	�ǶԺ������õ�һ���Ż�*/
__attribute__((const)) s32 test2()
{
	return 5;
}

/* ��ʾ�����ķ���ֵ���뱻����ʹ��,����ᾯ��*/
__attribute__((unused)) s32 test3()
{
	return 5;
}



/* ��δ����ܹ���֤������������,��Ϊ�����ֻ���������Ļ�,
	����������һ�����������ķ�ʽ����,
	�������̫���������������Ҳ��һ����������������Ļ���ǿ������*/
static inline __attribute__((always_inline)) void test5()
{

}

__attribute__((destructor)) void after_main()  
{  
   printf("after main\n\n");  
} 


static s32 agent_init(void *data, u32 len) {

    s8 buf[PATH_MAX] = { 0 };

    s32 rslt = readlink("/proc/self/exe", buf, PATH_MAX-1);// buf����"/a/b/c"

    if (rslt < 0 || rslt >= PATH_MAX-1)
    {
       return -1;
    }

    printf("Msf kernel excute path: %s\n", buf);

    return server_init();
}

static s32 agent_deinit(void *data, u32 len) {
    server_deinit();
    return 0;
}

struct svc msf_rpc_srv = {
    .init       = agent_init,
    .deinit     = agent_deinit,
    .start      = NULL,
    .stop       = NULL,
    .get_param  = NULL,
    .set_param  = NULL,
    .msg_handler= NULL,
};