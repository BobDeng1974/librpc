#include "gipc_palloc.h"
#include "common.h"

static void *ngx_palloc_block(ngx_pool_t *pool, unsigned int size);
static void *ngx_palloc_large(ngx_pool_t *pool, unsigned int size);

/*
 *	ngx_create_pool��	����pool
 *	ngx_destory_pool��	���� pool
 *	ngx_reset_pool��	����pool�еĲ�������
 *	ngx_palloc/ngx_pnalloc����pool�з���һ���ڴ�
 *	ngx_pool_cleanup_add��	Ϊpool���cleanup����
*/

ngx_pool_t* ngx_create_pool(unsigned int size)
{
    ngx_pool_t  *p;

    p = gipc_memalign(NGX_POOL_ALIGNMENT, size);
    if (p == NULL) {
        return NULL;
    }

    p->d.last 	= (unsigned char *) p + sizeof(ngx_pool_t);
    p->d.end 	= (unsigned char *) p + size;
    p->d.next 	= NULL;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_t);
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = NULL;
	//p->chain = NULL;
    p->cleanup = NULL;

    return p;
}


void ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
	ngx_pool_cleanup_t	*c;


    for (c = pool->cleanup; c; c = c->next) { 
        if (c->handler) {
            printf("ngx_destroy_pool run cleanup: %p\n", c);
            c->handler(c->data);
        }
    }

    for (l = pool->large; l; l = l->next) {
		sfree(l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        sfree(p);
        if (n == NULL) {
            break;
        }
    }
}


void ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        sfree(l->alloc);
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (unsigned char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->large = NULL;
    //pool->chain = NULL;

}

/* ���Ի������ڴ�ض���,��Щ�ڴ�صĴ�С,���弰�����ڸ�����ͬ.
 * ��3���ֻ��漰����ڴ��,����ʹ��r->pool�ڴ�ؼ���.
 * ����ngx_pool_t�����,���Դ��ڴ���з����ڴ�.
 * ����,ngx_palloc���������pool�ڴ���з��䵽size�ֽڵ��ڴ�,
 * ����������ڴ����ʼ��ַ.�������NULL��ָ�룬���ʾ����ʧ��.
 * ����һ����װ��ngx_palloc�ĺ���ngx_pcalloc,��������һ����,
 * ���ǰ�ngx_palloc���뵽���ڴ��ȫ����Ϊ0,��Ȼ,��������¸��ʺ�
 * ��ngx_pcalloc�������ڴ档
*/

void* ngx_palloc(ngx_pool_t *pool, unsigned int size)
{
    unsigned char*	m;
    ngx_pool_t*		p;

    /* �ж� size �Ƿ���� pool ����ʹ���ڴ��С */
    if (size <= pool->max) {

		/* ��current���ڵ�pool���ݽڵ㿪ʼ����
			����Ѱ���Ǹ��ڵ���Է���size�ڴ� */
        p = pool->current;

        do {
            m = gipc_align_ptr(p->d.last, NGX_POOL_ALIGNMENT);

			/* �ж�pool��ʣ���ڴ��Ƿ��� */
            if ((unsigned int) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

			/* �����ǰ�ڴ治��,������һ���ڴ���з���ռ� */
            p = p->d.next;

        } while (p);

	/*
	 *  �����������һ�����,����Ҫ���ڴ����pool���ɷ����ڴ��Сʱ,
	 *  ��ʱ�����ж�size�Ѿ�����pool->max�Ĵ�С��,����ֱ�ӵ���
	 *  ngx_palloc_large���д��ڴ���䣬���ǽ�ע����ת���������
	 *  ��ƪ������Դ�� Linux������վ(www.linuxidc.com)  
	 *  ԭ�����ӣ�http://www.linuxidc.com/Linux/2011-08/41860.htm
	  */

        return ngx_palloc_block(pool, size);
    }



    return ngx_palloc_large(pool, size);
}


void* ngx_pnalloc(ngx_pool_t *pool, unsigned int size)
{
    unsigned char      *m;
    ngx_pool_t  *p;


    if (size <= pool->max) {

        p = pool->current;

        do {
            m = p->d.last;

            if ((unsigned int) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return ngx_palloc_block(pool, size);
    }


    return ngx_palloc_large(pool, size);
}

/* ���ǰ�濪�ٵ�pool�ռ��Ѿ����꣬����¿��ٿռ�ngx_pool_t */
static void* ngx_palloc_block(ngx_pool_t *pool, unsigned int size)
{
    unsigned char* m;
    unsigned int   psize;
    ngx_pool_t *p, *new;

    psize = (unsigned int) (pool->d.end - (unsigned char *) pool);

    m = gipc_memalign(NGX_POOL_ALIGNMENT, psize);
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = gipc_align_ptr(m, NGX_POOL_ALIGNMENT);
    new->d.last = m + size;

	/* �ж��ڵ�ǰ pool�����ڴ��ʧ�ܴ���,��:���ܸ��õ�ǰ pool �Ĵ���,
	 * ������� 4 ��,������ڴ� pool ���ٴγ��Է����ڴ�,�����Ч��
	 * ���ʧ�ܴ�������4(������4),�����currentָ��,
	 * ��������pool���ڴ������ʹ��.
	 */

    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = new;

    return m;
}


/*
 * ����Ҫ���ڴ����pool���ɷ����ڴ��Сʱ,
 * ��ʱ�����ж�size�Ѿ�����pool->max�Ĵ�С��,
 * ����ֱ�ӵ���ngx_palloc_large���д��ڴ����,
 * ��ƪ������Դ�� Linux������վ(www.linuxidc.com)  
 * ԭ�����ӣ�http://www.linuxidc.com/Linux/2011-08/41860.htm
*/
static void* ngx_palloc_large(ngx_pool_t *pool, unsigned int size)
{
    void              *p;
    int               n;
    ngx_pool_large_t  *large;

	/* ��������һ���СΪ size �����ڴ�
     * ע��:�˴���ʹ�� ngx_memalign ��ԭ����,�·�����ڴ�ϴ�,
     * ����Ҳû̫���Ҫ ���Һ����ṩ�� ngx_pmemalign ����,
     * ר���û���������˵��ڴ� */
    p = gipc_alloc(size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

	/* ����largt�����Ͽ����large ָ�� */
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

		 /*
          * �����ǰ large �󴮵� large �ڴ����Ŀ���� 3 (������3)
          * ��ֱ��ȥ��һ���������ڴ�,���ٲ�����.
          */
        if (n++ > 3) {
            break;
        }
    }

    large = ngx_palloc(pool, sizeof(ngx_pool_large_t));
    if (large == NULL) {
		sfree(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void* ngx_pmemalign(ngx_pool_t *pool, unsigned int size, unsigned int alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    p = gipc_memalign(alignment, size);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc(pool, sizeof(ngx_pool_large_t));
    if (large == NULL) {
		sfree(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

/* �ú���ֻ�ͷ�large������ע����ڴ�,��ͨ�ڴ���ngx_destroy_pool��ͳһ�ͷ� */
int ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            sfree(l->alloc);
            return 1;
        }
    }

    return 0;
}

void* ngx_pcalloc(ngx_pool_t *pool, unsigned int size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        gipc_memzero(p, size);
    }

    return p;
}



ngx_pool_cleanup_t* ngx_pool_cleanup_add(ngx_pool_t *p, unsigned int size)
{
    ngx_pool_cleanup_t  *c;

    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    return c;
}


void ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

/*
Nginx���첽�ؽ������ļ���Ч�ط��͸��û����������Ǳ���Ҫ��HTTP�������Ӧ������Ϻ�ر��Ѿ��򿪵��ļ���������򽫻���־��й¶���⡣
���������ļ����Ҳ�ܼ򵥣�ֻ��Ҫ����һ��ngx_pool_cleanup_t�ṹ�壨������򵥵ķ�����HTTP��ܻ��ṩ��������ʽ�����������ʱ�ص�����HTTPģ���cleanup���������ڵ�11�½��ܣ���
�����Ǹյõ����ļ��������Ϣ������������Nginx�ṩ��ngx_pool_cleanup_file�������õ�����handler�ص������м��ɡ�

*/
//ngx_pool_cleanup_file�������ǰ��ļ�����رա��������ʵ���п��Կ�����ngx_pool_cleanup_file������Ҫһ��ngx_pool_cleanup_file_t���͵Ĳ�����
//��ô������ṩ��������أ���ngx_pool_cleanup_t�ṹ���data��Ա�ϸ�ֵ���ɡ��������һ��ngx_pool_cleanup_file_t�Ľṹ��

/*
���Կ�����ngx_pool_cleanup_file_t�еĶ�����ngx_buf_t��������file�ṹ���ж����ֹ��ˣ�����Ҳ����ͬ�ġ�����file�ṹ�壬�������ڴ�����Ѿ�Ϊ��������ڴ棬
ֻ�����������ʱ�Ż��ͷţ���ˣ�����򵥵�����file��ĳ�Ա���ɡ������ļ�����������������¡�
ngx_pool_cleanup_t* cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_pool_cleanup_file_t));
if (cln == NULL) {
 return NGX_ERROR;
}

cln->handler = ngx_pool_cleanup_file;
ngx_pool_cleanup_file_t  *clnf = cln->data;

clnf->fd = b->file->fd;
clnf->name = b->file->name.data;
clnf->log = r->pool->log;

ngx_pool_cleanup_add���ڸ���HTTP��ܣ����������ʱ����cln��handler����������Դ��
*/
void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    sclose(c->fd);
}


void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    int  err;

    printf("file cleanup: fd:%d %s\n", c->fd, c->name);

 
	unlink(c->name);
	sclose(c->fd);
}


void ngx_pool_status(ngx_pool_t* pool)  
{  
    int n = 0;  
    ngx_pool_t *p = pool;  
  
    printf("**********************************************************************\n");  
    for(; p; p = p->d.next, n++)  
    {  
        printf("pool:%d  ", n);  
        printf("max:%d  left:%d\n", p->max, p->d.end - p->d.last);  
    }  
    printf("**********************************************************************\n");  
}  

#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}
#endif





