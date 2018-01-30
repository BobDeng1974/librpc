#include "gipc_alloc.h"

/* ngx_os_init����һ����ҳ�Ĵ�С,��λΪ�ֽ�(Byte).
 * ��ֵΪϵͳ�ķ�ҳ��С,��һ�����Ӳ����ҳ��С��ͬ 
 * ngx_pagesizeΪ4M��ngx_pagesize_shiftӦ��Ϊ12
 * ngx_pagesize������λ�Ĵ���,��for (n = ngx_pagesize; 
 * n >>= 1; ngx_pagesize_shift++) { void }
 */
//#define ngx_os_pagesize (getpagesize() - 1)
unsigned int  gipc_pagesize = 4096; 
unsigned int  gipc_pagesize_shift; 

/* �����֪��CPU cache�еĴ�С,��ô�Ϳ���������Ե������ڴ�Ķ���ֵ,
 * ����������߳����Ч��.Nginx�з����ڴ�صĽӿ�,Nginx�Ὣ�ڴ�ر߽�
 * ���뵽 CPU cache�д�С 32λƽ̨,ngx_cacheline_size=32
 */
//unsigned int  gipc_cacheline_size = 32;


void* gipc_alloc(unsigned int size)
{
    void* p;

    p = malloc(size);
	if ( !p ) {
		printf("malloc size [%d] failed\n", size);
	}

    return p;
}


void* gipc_calloc(unsigned int size)
{
    void* p;

    p = gipc_alloc(size);

    if (p) {
        gipc_memzero(p, size);
    }

    return p;
}

void* ngx_realloc(void *p, unsigned int size){

    if(p) {
        return realloc (p, size);
    }

    return NULL;

}

#if (GIPC_HAVE_POSIX_MEMALIGN)

void* gipc_memalign(unsigned int alignment, unsigned int size)
{
    void* p   = NULL;
    int   err = -1;

	/*void* p = memalign(alignment, size) */
	
    err = posix_memalign(&p, alignment, size);
	if ( err ) {
		printf("posix_memalign(%uz, %uz) failed\n", alignment, size);
	}

    return p;
}

#elif (GIPC_HAVE_MEMALIGN)

void* gipc_memalign(unsigned int alignment, unsigned int size) {

	void* p;

    p = memalign(alignment, size);
	if ( !p ) {
		printf("memalign(%uz, %uz) failed\n", alignment, size);
	}

	return p;
}

#endif

