#include "gipc_alloc.h"


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (gipc_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    	gipc_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)

typedef void (*ngx_pool_cleanup_pt)(void *data);
typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;


/*
 * �ڴ��       ---  ngx_pool_s
 * �ڴ������   ---  ngx_pool_data_t
 * ���ڴ��     --- ngx_pool_large_s;
 */

typedef struct ngx_pool_large_s ngx_pool_large_t;

typedef struct ngx_pool_s ngx_pool_t;

struct ngx_pool_cleanup_s { 
    ngx_pool_cleanup_pt handler;

	/*�ڴ��������ַ �ص�ʱ,�������ݴ���ص�����*/
    void*				data;
    ngx_pool_cleanup_t*	next;
};

struct ngx_pool_large_s {
    ngx_pool_large_t* 	next;
    void*				alloc;
};

typedef struct {
	/* ��������ڴ��β��ַ,��������׵�ַ 
	   pool->d.last ~ pool->d.end �е��ڴ������ǿ��������� */
    unsigned char*	last;
	/* ��ǰ�ڴ�ؽڵ����������ڴ������λ�� */
    unsigned char*	end;

	/* ��һ���ڴ�ؽڵ�ngx_pool_t,��ngx_palloc_block */
    ngx_pool_t*		next;

	/*���ִӵ�ǰpool�з����ڴ�ʧ���Ĵ�,��ʹ����һ��pool,��ngx_palloc_block*/ 
    unsigned int 	failed;
} ngx_pool_data_t;

/* Ϊ�˼����ڴ���Ƭ������,��ͨ��ͳһ���������ٴ����г����ڴ�й©�Ŀ�����,
 * Nginx�����ngx_pool_t�ڴ�����ݽṹ��
 * ͼ�λ����ο�Nginx�ڴ�ط���:
 * http://www.linuxidc.com/Linux/2011-08/41860.htm
 */
struct ngx_pool_s {
	/* �ڵ�����  ���� pool ��������ָ��Ľṹ�� 
	   pool->d.last ~ pool->d.end �е��ڴ������ǿ��������� */
    ngx_pool_data_t 	d;	

	/* ��ǰ�ڴ�ڵ�������������ڴ�ռ� һ������pool�п��ٵ����ռ� */
    unsigned int		max;
	/* �ڴ���п��������ڴ�ĵ�һ���ڵ� */
    ngx_pool_t*			current;

	//ngx_chain_t*		chain
	/* �ڵ��д��ڴ��ָ�� pool ��ָ������ݿ��ָ��
	   �����ݿ���ָ size > max �����ݿ� */
    ngx_pool_large_t*	large;

	/* pool��ָ�� ngx_pool_cleanup_t ���ݿ��ָ�� 
	   cleanup��ngx_pool_cleanup_add��ֵ */
    ngx_pool_cleanup_t*	cleanup;
};


typedef struct {
    int     fd;
    char*	name;
} ngx_pool_cleanup_file_t;


void* ngx_alloc(unsigned int size);
void* ngx_calloc(unsigned int size);

ngx_pool_t* ngx_create_pool(unsigned int size);
void ngx_destroy_pool(ngx_pool_t* pool);
void ngx_reset_pool (ngx_pool_t* pool);
void ngx_pool_status(ngx_pool_t* pool);


void *ngx_palloc(ngx_pool_t *pool, unsigned int size);
void *ngx_pnalloc(ngx_pool_t *pool, unsigned int size);
void *ngx_prealloc(ngx_pool_t *pool, void *p, unsigned int size);
void *ngx_pcalloc(ngx_pool_t *pool, unsigned int size);
void *ngx_pmemalign(ngx_pool_t *pool, unsigned int size, unsigned int alignment);
int   ngx_pfree (ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, unsigned int size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);

