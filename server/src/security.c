#include <unistd.h>

#include <stdio.h>
#include <string.h>


#if 0
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>

#define	KMC_UPDATE	1	
#define KMC_VERIFY	2
#define KMC_REVOKE	3 /* ע�� */


typedef struct {

	X509_CRL *crl;
	X509_REVOKED *revoked;

	X509_NAME *issuer;
	ASN1_TIME *lastUpdate;
	ASN1_TIME *nextUpdate;
	ASN1_TIME *rvTime;

	EVP_PKEY *pkey;


} kmc_crl_t;

typedef struct  __attribute__((__packed__)) {
	BIGNUM *bne;

	RSA		*rsa; /* �洢��˽Կ�� */
	unsigned long	e;
	int32 	rsa_key_bits;

	

	RSA 	*public_key;
    RSA 	*private_key;

	int8 	*password;

	X509_NAME *issuer_root;//root_cert--->ca
	X509_NAME *issuer_ca;/* ===> server, client, crl */

	X509 	*x509_root;
	EVP_PKEY *root_public_key; //��Կ
	EVP_PKEY *root_private_key; //˽Կ
	
	X509 	*x509_ca;
	EVP_PKEY *ca_public_key; //��Կ
	EVP_PKEY *ca_private_key; //˽Կ
	
	X509 	*x509_server;
	EVP_PKEY *server_private_key; //˽Կ
	EVP_PKEY *server_public_key; //��Կ
	long 	server_sn;
	long 	server_days;
	
	X509 	*x509_client;
	EVP_PKEY *client_public_key; //��Կ
	EVP_PKEY *client_private_key; //˽Կ
	
} kmc_t;

static kmc_t kmc_server;
static kmc_t *kmc = &kmc_server;


typedef struct  __attribute__((__packed__)) {
	uint32	cmd_type;
	int8	client_id[32];
	int8	server_id[32];
} kmc_request;


int32 kmc_handle_message();

int32 kmc_handle_update();
int32 kmc_handle_verify();
int32 kmc_handle_revoke();


int32 kmc_gen_server_cert(kmc_t *kmc);
int32 kmc_gen_crl_cert(kmc_crl_t *kmc);
int32 kmc_gen_key_pairs(kmc_t *kmc);


int32 kmc_init(void) {

	kmc->password = "luotang.me";

	kmc->e = RSA_3;
	kmc->rsa_key_bits = 1024;

	kmc->issuer_cert = X509_NAME_new();
	kmc->x509_root = X509_new();
	kmc->x509_ca = X509_new();
	kmc->x509_client = X509_new();

	kmc_gen_key_pairs(kmc);

	kmc_crl_t *kmc_crl;

	kmc_gen_crl_cert(kmc_crl);

	return 0;
}


#define ROOT_PUBKEY_PATH 	"/home/luotang.me/app/root_pub_key.key"
#define ROOT_PRIKEY_PATH 	"/home/luotang.me/app/root_pri_key.key"
#define CA_PUBKEY_PATH 		"/home/luotang.me/app/ca_pub_key.key"
#define CA_PRIKEY_PATH 		"/home/luotang.me/app/ca_pri_key.key"
#define SERVER_PUBKEY_PATH 	"/home/luotang.me/app/server_pub_key.key"
#define SERVER_PRIKEY_PATH 	"/home/luotang.me/app/server_pri_key.key"
#define CLIENT_PUBKEY_PATH 	"/home/luotang.me/app/client_pub_key.key"
#define CLIENT_PRIKEY_PATH 	"/home/luotang.me/app/client_pri_key.key"


//����1024λ��RSA˽Կ
//openssl genrsa -out private.key 1024

//����˽Կ���ɹ�Կ
//openssl rsa -in private.key -pubout -out public.key

//˽Կ�ļ�private.key
//��Կ�ļ�public.key
//����˽Կ��û���ܵ�,��ѡ����,ָ��һ�������㷨����ʱ��������


int32 kmc_gen_key_pairs_t(kmc_t *kmc, 
	EVP_PKEY *pubkey, EVP_PKEY *prikey, int8* pubkey_path, int8* prikey_path) {

	pubkey = EVP_PKEY_new();
	if (!pubkey) goto error;
	
	EVP_PKEY_assign_RSA(pubkey, kmc->rsa);

  	BIO *bioPtr = BIO_new(BIO_s_file());
	if (BIO_write_filename(bioPtr, pubkey_path) <= 0 )
	{
		perror ("failed to open public key file ") ;
		goto error;
	}

	//----- write public key into file -----
	if (PEM_write_bio_RSAPublicKey(bioPtr, kmc->rsa) != 1)
	{
	 	perror ("failed to write RSA public key into file") ;
		goto error;
	}
	BIO_flush(bioPtr);
	BIO_free_all(bioPtr) ; // don't forget release and free the allocated space

  	printf ("generated RSA public key already written into file %s \n" , kmc->public_key_path);

	//----- private key -----
	prikey = EVP_PKEY_new();
	if (!prikey) goto error;

	EVP_PKEY_assign_RSA(prikey, kmc->rsa);
    
   	bioPtr = BIO_new_file(prikey_path, "w+") ;
   	if ( bioPtr == NULL )
   	{
    	perror ("failed to open file stores RSA private key ") ;
       	goto error;
   	} 

   //�������ɵ�˽Կû�м��ܣ���ѡ����
   	if (PEM_write_bio_RSAPrivateKey(bioPtr, kmc->rsa, EVP_des_ede3_ofb() ,
            (uint8*)kmc->password, strlen(kmc->password) , NULL , NULL ) != 1)
   //if (PEM_write_bio_RSAPrivateKey (bioPtr, kmc->rsa, NULL,
   //        NULL, 0 , NULL , NULL ) != 1)
   	{
    	perror ("failed write RSA private key into file") ;
    	goto error;
   	}
   	BIO_flush(bioPtr);
   	BIO_free_all(bioPtr) ; // do not forget this 
   
   	printf ("genertated RSA private key already written into file %s \n",
		kmc->private_key_path) ;
	return 0;
error:
	RSA_free(kmc->rsa);
	BN_free(kmc->bne);
	return -1;
}


int32 kmc_gen_key_pairs(kmc_t *kmc) {

	int32 rc = -1;

	rc = kmc_gen_key_pairs_t(kmc, kmc->root_public_key, kmc->root_prilic_key,
			ROOT_PUBKEY_PATH, ROOT_PRIKEY_PATH);

	rc = kmc_gen_key_pairs_t(kmc, kmc->ca_public_key, kmc->ca_prilic_key,
			CA_PUBKEY_PATH, CA_PRIKEY_PATH);

	rc = kmc_gen_key_pairs_t(kmc, kmc->server_public_key, kmc->server_prilic_key,
			SERVER_PUBKEY_PATH, SERVER_PRIKEY_PATH);

	rc = kmc_gen_key_pairs_t(kmc, kmc->client_public_key, kmc->client_prilic_key,
			CLIENT_PUBKEY_PATH, CLIENT_PRIKEY_PATH);

	///////////////////////////////////////////////
	// public key
	
	return -1;
}

int32 kmc_get_key_pairs(kmc_t *kmc) {
	//������BIO
	RSA *pubkey = RSA_new();
    RSA *prikey = RSA_new();

    BIO *pubio;
    BIO *priio;
    
    priio = BIO_new_file(kmc->ca_private_key, "rb");
    prikey = PEM_read_bio_RSAPrivateKey(priio, &prikey, NULL, NULL);
    
    pubio = BIO_new_file(kmc->ca_private_key, "rb");
    pubkey = PEM_read_bio_RSAPublicKey(pubio, &pubkey, NULL, NULL);
    
    RSA_print_fp(stdout, pubkey, 0);
    RSA_print_fp(stdout, prikey, 0);

    RSA_free(pubkey);
    BIO_free(pubio);
    RSA_free(prikey);
    BIO_free(priio);


	//FILE
	pubkey = RSA_new();
    prikey = RSA_new();
    FILE *pubf = fopen(kmc->public_key_path, "rb");
    pubkey = PEM_read_RSAPublicKey(pubf, &pubkey, NULL, NULL);
    
    FILE *prif = fopen(kmc->private_key_path, "rb");
    prikey = PEM_read_RSAPrivateKey(prif, &prikey, NULL, NULL);
    
    RSA_print_fp(stdout, pubkey, 0);
    RSA_print_fp(stdout, prikey, 0);

    fclose(pubf);
    fclose(prif);
    RSA_free(pubkey);
    RSA_free(prikey);

	return 0;
}

int32 kmc_gen_server_cert(kmc_t *kmc) {

	kmc->x509_server = X509_new();
	kmc->server_public_key = EVP_PKEY_new();
		
	//����֤��Ĺ�Կ��Ϣ
	X509_set_pubkey(kmc->x509_server, kmc->server_public_key);

	//���ð汾��
	X509_set_version(kmc->x509_server, 3);

	kmc->server_sn = 12345689;
	//����֤�����кţ����sn����CA���İ䷢�ĵ�N��֤��
	ASN1_INTEGER_set(X509_get_serialNumber(kmc->x509_server), kmc->server_sn);

	//����֤�鿪ʼʱ��
	X509_gmtime_adj(X509_get_notBefore(kmc->x509_serve), 0);

	kmc->server_days = 360;
	//����֤�����ʱ��
	X509_gmtime_adj(X509_get_notAfter(kmc->x509_serve), 
					(long)60*60*24* kmc->server_days) ;
	
	//����֤���ǩ������Ϣ
	X509_set_issuer_name(kmc->x509_server, X509_get_subject_name(kmc->x509_ca));

	
	// ��������
	X509_NAME *subject = X509_get_subject_name(kmc->x509_server);
	// ����
	X509_NAME_add_entry_by_txt(subject, SN_countryName, MBSTRING_UTF8,
							   (unsigned char *)"CN", -1, -1, 0);
	// ʡ��
	X509_NAME_add_entry_by_txt(subject, SN_stateOrProvinceName,  MBSTRING_UTF8,
							   (unsigned char *)"Sichuan", -1, -1, 0);
	// ����
	X509_NAME_add_entry_by_txt(subject, SN_localityName,  MBSTRING_UTF8,
							   (unsigned char *)"Chengdu", -1, -1, 0);

	
    X509_set_subject_name(kmc->x509_server, subject);

	// ������չ��Ŀ
	X509V3_CTX ctx;
	X509V3_set_ctx(&ctx, kmc->x509_ca, subject, NULL, NULL, 0);
	
	char *name = "luotang.me";
	char *value = "luotang"; 
	X509_EXTENSION *x509_ext = X509_EXTENSION_new();
	x509_ext = X509V3_EXT_conf(NULL, &ctx, name, value);
	X509_add_ext(kmc->x509_server, x509_ext, -1);
	
	// ����ǩ��ֵ
	X509_sign(kmc->x509_server, kmc->ca_private_key, EVP_md5());
	// ����һ��X509֤��������ˣ������������Ƕ������б��뱣�档
	//i2d_X509_bio(pbio, kmc->x509_server); //DER��ʽ
	//PEM_write_bio_X509(pbio, kmc->x509_server); //PEM��ʽ
	 
}


int32 cert_get_serial(int8 *cert, int32 cert_len, int32 cert_format) {
	
	X509 *x509;
	int8 *serial = i2s_ASN1_INTEGER(NULL, X509_get_serialNumber(x509));
	X509_free(x509);
	
	return atoi(serial);
}

int32 cert_get_ca_public_key(X509 *x509, int8 *pubkey, int32 *pubkey_len) {

	//*pubkey_len = x509->cert_info->key->public_key->length - 1;
	//memcpy((char*)pubkey, x509->cert_info->key->public_key->data + 1, *pubkey_len);	
	return 0;
}

//У��RSA Key�ĳ���
bool valid_rsa_size(X509 *cert) {
    // key length valid
    EVP_PKEY *pkey;
    RSA *rsa;
    int rsa_size;
    pkey = X509_get_pubkey(cert);
    rsa = EVP_PKEY_get1_RSA(pkey);
    if (!rsa) {
        return false;
    }
	#define  RSAKEY_MIN_LEN 100
    rsa_size = RSA_size(rsa);
    if (rsa_size * 8 < RSAKEY_MIN_LEN) {
        return false;
    }
    return true;
}

X509* kmc_cert_load(int8 *cert_path) {
	
	FILE *fp = fopen(cert_path, "rb");
	if ( NULL == fp)
    {
        fprintf(stderr, "fopen %s fail \n", cert_path);
        return -1;
    }
	
	fseek(fp, 0, SEEK_END);
	long int cert_len = ftell(fp);

	printf( "%s ftell size %ld\n", cert_path, cert_len);
	if(0 == cert_len){
		return -1;
	}
	fseek(fp, 0, SEEK_SET);
	char* cert = (char*)malloc(cert_len+1);
	
	fread(cert, 1, cert_len, fp);
	
	char* pcert = cert;//���봫��ָ��
	
	X509* x509_t = d2i_X509(NULL, (const uint8 **)&pcert, cert_len);	

    fclose(fp);

	return x509_t;
}

/* ��ȡ֤��Ŀ�ʼ�ͽ���ʱ�� */
int32 kmc_cert_life(X509 *x509, time_t *notBefore, time_t *notAfter) {
	int err;
    ASN1_STRING *before, *after;
    
    if (!x509 || !notBefore || !notAfter)
		return -1;
   
    
    before = X509_get_notBefore(x509);

	ASN1_UTCTIME *be=ASN1_STRING_dup(before);
    after = X509_get_notAfter(x509);
	ASN1_STRING *af=ASN1_STRING_dup(after);
	/*  ��ʱ���ʽת��Ϊtime_t��ʽ�����Ҽ��뵱ǰ�豸���ڵ�ʱ�� */
   // *notBefore = ASN1_TIME_get(before, &err);
   // *notAfter = ASN1_TIME_get(after, &err);

	//��ǰʱ��Ƚ�

	int day = 0;
    int sec = 0;
    ASN1_TIME_diff(&day, &sec, notBefore, NULL);

	ASN1_TIME_diff(&day, &sec, NULL, notAfter);//��֤��Ľ���ʱ���뵱ǰʱ��Ƚ�

	return 0;
}



// ��ȡ֤���ǩ���㷨
int32 get_cert_alg(X509 *x509) {

   char buf[256];
   memset(buf,0,256);
   //i2t_ASN1_OBJECT(buf, 1024, x509->cert_info->signature->algorithm);
   X509_free(x509);
   return 0;

}

//֤�鳷���б�Certificate Revocation List, ���CRL��
/*
https://www.cnblogs.com/274914765qq/p/4613007.html

https://blog.csdn.net/aixiaoxiaoyu/article/details/79175859

CRL��֤�鳷��״̬�Ĺ�����ʽ��CRL�������ÿ��ĺ�������
���ڹ���ĳЩ����֤�鲻����Ч��
CRL��һ�����ߵ�֤��״̬��Ϣ��
��һһ�������ڽ��и��¡�CRL���Է�Ϊ��ȫCRL������CRL��
����ȫCRL�а��������еı�����֤����Ϣ������CRL��һЩ
�е�CRL��������������֤����Ϣ����ÿ�η�����CRL�Ƕ�ǩ��
����CRL����������
������CRL��Ϣ�ɣ�
������֤�����к�
����ʱ��
����ԭ��
ǩ����
CRLǩ����


X509V3����֤����Ҫ������������:
a.֤��汾
b.֤�����к�
c.ǩ���㷨
d.�䷢����Ϣ
e.��Чʱ��
f.��������Ϣ
g.��Կ��Ϣ
h.�䷢����Ϣ
i.��Կ��Ϣ
j.�䷢��ID
k.��չ��


ROOT֤�顢CA֤���ʹ��CA֤ǩ����X.509֤��֮�����ϵ��
��ʵ����һ��ʹ��˽Կǩ����issue���Ĺ�ϵ��
ʹ��ROOT֤���˽Կǩ��CA֤�飬��ʹ��CA֤��ǩ��������X.509֤�飬
�������γ���һ�������ŵ�Path��
*/
int32 cert_get_crl_info(X509 *x509_t) {

	int ver = X509_get_version(x509_t);
	switch(ver)       
	{  
	    case 0:     //V1  
	        //...  
	    break;  
	    case 1:     //V2  
	        //...  
	    break;  
	    case 2:     //V3  
	        //...  
	    break;  
	    default:  
	        //Error!  
	    break;  
	}  

	X509_get_pubkey(x509_t);
	return 0;
}
  
int32 cert_get_crl_ext_info(X509 *x509_t)
{
	//CRL֤����չ ����֤��
	int pos = X509_get_ext_by_NID(x509_t, NID_crl_distribution_points, -1);
	X509_EXTENSION *pExt = X509_get_ext(x509_t, pos);

	CRL_DIST_POINTS *crlpoints = (CRL_DIST_POINTS*)X509_get_ext_d2i(x509_t, 
		NID_crl_distribution_points, &pos, NULL);
	if (!crlpoints) {
		
	}
	
	int i;
	for (i = 0; i < sk_DIST_POINT_num(crlpoints); i++)
	{
		int j, gtype;
		ASN1_STRING *uri;
		GENERAL_NAMES *gens;
		GENERAL_NAME *gen;
		DIST_POINT *dp = sk_DIST_POINT_value(crlpoints, i);
		if (!dp->distpoint || dp->distpoint->type != 0)
			continue;

		gens = dp->distpoint->name.fullname;
		for (j = 0; j < sk_GENERAL_NAME_num(gens); j++)
		{
			gen = sk_GENERAL_NAME_value(gens, j);
			uri = (ASN1_STRING*)GENERAL_NAME_get0_value(gen, &gtype);
			if (gtype == GEN_URI && ASN1_STRING_length(uri) > 6)
			{
				int8 *crl_url = (int8 *)ASN1_STRING_data(uri);
				printf("CRL updata url: %s \n", crl_url);
				break;

			}
		}// end for
	}

	
	X509_free(x509_t);
	return 0;
}

///* 2 ��֤����֤���ǩ��ֵ */

/* ��֤����֤���Ƿ���CRL֤����,������֤CRL֤�����Ч�� */

//2.֤����Ч�ڣ��Ƿ����
//3.֤��ǩ�����Ƿ�ΪCA��֤��ǩ��  DN��

static int gbCheck_Issuer(X509_NAME* name1, X509_NAME* name2)
{	
	if(!name1 || !name2){
		return -1;
	}

	int i = 0;
	int iType[] = {
		NID_commonName,
		NID_countryName,
		NID_localityName,
		NID_stateOrProvinceName,
		NID_organizationName,
		NID_organizationalUnitName,
	};
	char pCA[128];
	char pSign[128];
	
	for (i = 0; i < sizeof(iType)/sizeof(int); i++)
	{
		memset(pCA, 0, sizeof(pCA));
		memset(pSign, 0, sizeof(pSign));
		
		X509_NAME_get_text_by_NID(name1, iType[i], pCA, 128);
		X509_NAME_get_text_by_NID(name2, iType[i], pSign, 128);

		if (0 != memcmp(pCA, pSign, 128)) //��ǰ��ֵ��һ��
		{
			printf( "X509 pCA not match pSign\n");
			i = -1;
			break;
		}
	}
	if(-1 == i){
		return -1;
	}

	return 0;
}


/* ��֤����֤���ǩ��ֵ */



/* ��֤����֤���Ƿ���CRL֤����,������֤CRL֤�����Ч�� */

int gbCheck_Server_Cert_Crl()
{
	printf("gbCheck_Server_Cert_Crl start\n");

	//CRL����ʱ������CRLʧ�ܣ��������þɵ�CRL
	//#define CRL_URL "http://10.8.7.105/download.crl"
	#define CRL_PATH "/devinfo/gb.crl"
	
	int ret = 0;
	int bUpdateCrl = false;

	FILE* fp = NULL;

	STACK_OF(X509_REVOKED) *rev;
	X509_CRL* x509_crl = NULL;

	do{
		//����CRLʧ�ܻ����CRLʧ��,Ҳ��Ĭ�ϳɹ����������޸�
		if(access(CRL_PATH, F_OK)) {  
			printf( "/devinfo/gb.crl Not Exist \n"); 
			
		}
		fp = fopen(CRL_PATH, "rb");
		if(!fp){
			printf("gb.crl fopen error\n");
			ret = 0;
			break;
		}

		fseek(fp, 0, SEEK_END);
		long int crlBufLen = ftell(fp);

		printf( "gb.crl ftell size %ld\n", crlBufLen);
		if(0 == crlBufLen){
			bUpdateCrl = 1;
			ret = 0;
			break;
		}
		fseek(fp, 0, SEEK_SET);
		char* crlCert = (char*)malloc(crlBufLen+1);
		if(NULL == crlCert){
			ret = 0;
			break;
		}
		//rewind (fp);

		int crlCertSize = fread(crlCert, 1, crlBufLen, fp);
		if(0 == crlCertSize){
			printf("gb.crl fread size 0\n"); 
			bUpdateCrl = 1;
			ret = 0;
			break;
		}
		char* pCrlBuf = crlCert;//���봫��ָ��
	
		x509_crl = d2i_X509_CRL(NULL, (const unsigned char **)&pCrlBuf, crlCertSize);
		if(!x509_crl){
			printf("d2i_X509_CRL failed\n");
			ret = 0;
			break;
		}
		//�õ�ת�����X509_CRL* 


		/* ��ȡ�汾*/
		long version = X509_CRL_get_version(x509_crl);
		printf("CRL Version %lu (0x%lx)\n", version + 1, version);
		
		
		X509_NAME *issuer = X509_CRL_get_issuer(x509_crl);	


		/*2. ��ȡ�ϴκ��´θ���ʱ�� */
		//���ctmʱ����now:cmp_time ֮���򷵻�ֵ����0
		ret  = X509_cmp_current_time(X509_CRL_get_nextUpdate(x509_crl));
		if (ret > 0) {
			ret = X509_cmp_current_time(X509_CRL_get_lastUpdate(x509_crl));
			if (ret < 0) {
				printf("X509_CRL_get_lastUpdate Invalid\n");
				bUpdateCrl = 1;
				ret = 0;
			}else{
				printf("X509_CRL_get_lastUpdate expired\n");
				bUpdateCrl = 1;
				ret = 0;
				//break;
				//��Ҫ����CRL�ļ� ���½ṹ������
			}		
		}else{
			printf("CRL_get_nextUpdate expired\n");
			bUpdateCrl = 1;
			ret = 0;
			//break;
		}


		/*1. ��ȡ�䷢����Ϣ  ��Ҫ��CA�ıȽϰ䷢��*/
		ret = gbCheck_Issuer(issuer, issuer);
		if(0 != ret){
			printf( "gbCheck_Issuer CA and CRL failed\n");
			ret = -1;
			break;
		}

		char* p = X509_NAME_oneline(issuer, NULL, 0);
		printf("CRL Issuer: %s\n", p);
		OPENSSL_free(p);

		//3. ��ȡǩ��ֵ
		
		
		/*4. ������֤�����к� */
		ASN1_INTEGER *serial = X509_get_serialNumber(x509_crl);
#if 0
		int idx;
		X509_REVOKED rtmp;
		rtmp.serialNumber = serial;
		
		//see def_crl_lookup
		rev = X509_CRL_get_REVOKED(x509_crl);
		idx = sk_X509_REVOKED_find(rev, &rtmp);
	    if (idx < 0){
			printf("Not find server serialNumber in crtList\n");
		}else{
			/* Need to look for matching name */
			//һ����������֤�� 
			X509_REVOKED *r;
			for (idx = 0; idx < sk_X509_REVOKED_num(rev); idx++) 
			{
				 r = sk_X509_REVOKED_value(rev, idx);//ret = sk_X509_REVOKED_pop(rev);
				  
				  //CRL entry EXTENSIONS
				  if(ASN1_INTEGER_cmp(serial, r->serialNumber))
				  {
			     	printf("not find server serialNumber in crt\n");
		     	  }
				  else
		     	  {
		     	  	  printf( "Find serverCert in crl Crllist !!!\n");
					  if (crl_revoked_issuer_match(x509_crl, issuer, r)) 
					  { 
					  	 //֤�����к� 
						 printf("Serial Number: %d\n", i2a_ASN1_INTEGER2(r->serialNumber));	 
						  
						 /* ��ȡ��������*/
						 printf("Revocation Date: ");
						 printf( "Sequence: %d\n", r->sequence);
						 printf("Reason: %d\n", r->reason);
						 
						 if (r->reason == CRL_REASON_REMOVE_FROM_CRL)
						 {
						 	printf("CRL_REASON_REMOVE_FROM_CRL\n");
						 }					
						ret = -1;
						break;				 
					  }

	 	  		  }
		 	}
		}

#endif
	}while(0);
	
	if(fp)
		fclose(fp);

	if(bUpdateCrl){
		printf("need to download CRL\n");
		
		
	}
	printf( "gbCheck_Server_Cert_Crl OK\n");
	
	return ret;

}


int i2a_ASN1_INTEGER2( ASN1_INTEGER *a)
{
    int i, n = 0;
    static const char *h = "0123456789ABCDEF";
    char buf[2];

    if (a == NULL)
        return (0);

    if (a->type & V_ASN1_NEG) {
     
        n = 1;
    }

    if (a->length == 0) {
       
        n += 2;
    } else {
        for (i = 0; i < a->length; i++) {
            if ((i != 0) && (i % 35 == 0)) {
               
                n += 2;
            }
            buf[0] = h[((unsigned char)a->data[i] >> 4) & 0x0f];
            buf[1] = h[((unsigned char)a->data[i]) & 0x0f];
           
            n += 2;
        }
    }
    return (n);
}

static int crl_revoked_issuer_match(X509_CRL *crl, X509_NAME *nm,
						X509_REVOKED *rev)
{
#if 0
	int i;

	if (!rev->issuer)
	{
		if (!nm)
			return -1;
		if (!X509_NAME_cmp(nm, X509_CRL_get_issuer(crl)))
			return -1;
		return 0;
	}

	if (!nm)
		nm = X509_CRL_get_issuer(crl);

	for (i = 0; i < sk_GENERAL_NAME_num(rev->issuer); i++)
	{
		GENERAL_NAME *gen = sk_GENERAL_NAME_value(rev->issuer, i);
		if (gen->type != GEN_DIRNAME)
			continue;
		if (!X509_NAME_cmp(nm, gen->d.directoryName))
			return -1;
	}
#endif
	return 0;

}



/////////////////////////////


int32 kmc_gen_crl_cert(kmc_crl_t *kmc_crl) {
	int32 ret, len;
    uint8 *buf,*p;
	
	
    FILE *fp;
    time_t t;
    X509_NAME *issuer;
    ASN1_TIME *lastUpdate,*nextUpdate,*rvTime;
    X509_CRL *crl=NULL;
    X509_REVOKED *revoked;

    ASN1_INTEGER *serial;

    BIO *bp;

	int32 rc = -1;
    
    /* ���ð汾*/
    kmc_crl->crl = X509_CRL_new();
	if (!kmc_crl->crl) return -1;
	
    rc = X509_CRL_set_version(crl, 3);
    
    /* ���ð䷢��*/
    issuer=X509_NAME_new();
    ret=X509_NAME_add_entry_by_NID(issuer,NID_commonName,V_ASN1_PRINTABLESTRING, "CRL issuer",10,-1,0);
    ret=X509_CRL_set_issuer_name(crl,issuer);
    
    /* �����ϴη���ʱ��*/
    lastUpdate=ASN1_TIME_new();
    t=time(NULL);
    ASN1_TIME_set(lastUpdate,t);
    ret=X509_CRL_set_lastUpdate(crl,lastUpdate);
    
    /* �����´η���ʱ��*/
    nextUpdate=ASN1_TIME_new();
    t=time(NULL);
    ASN1_TIME_set(nextUpdate,t+1000);
    ret=X509_CRL_set_nextUpdate(crl,nextUpdate);
    
    /* ��ӱ�����֤�����к�*/
    revoked=X509_REVOKED_new();
    serial=ASN1_INTEGER_new();
    ret=ASN1_INTEGER_set(serial,1000);
    ret=X509_REVOKED_set_serialNumber(revoked,serial);
    
    /* ���õ�������*/
    rvTime=ASN1_TIME_new();
    t=time(NULL);
    ASN1_TIME_set(rvTime,t+2000);
    ret=X509_CRL_set_nextUpdate(crl,rvTime);
    ret=X509_REVOKED_set_revocationDate(revoked,rvTime);

	///* ���һ��������֤�����Ϣ */ 
    ret=X509_CRL_add0_revoked(crl,revoked);

	//����֤�����кŶ�crl����,�˺���ʵ���˲����˶�ջ����,��ջ�ıȽϺ���ΪX509_REVOKED_cmp
    /* ����*/
    ret=X509_CRL_sort(crl);

	//��crl����ǩ��,pkeyΪ˽Կ,mdΪժҪ�㷨,��������x->signature��
    /* ǩ��*/
    ret=X509_CRL_sign(crl,kmc->ca_public_key,EVP_md5());
    
    /* д���ļ�*/
    bp=BIO_new(BIO_s_file());
    BIO_set_fp(bp,stdout,BIO_NOCLOSE);
    X509_CRL_print(bp,crl);
    len=i2d_X509_CRL(crl,NULL);
    buf=malloc(len+10);
    p=buf;
    len=i2d_X509_CRL(crl,&p);
    
    fp=fopen("raspberry.crl","wb");

	//��CRL����PEM��ʽ�ļ�
	//���ú���PEM_write_X509_CRL()
	//PEM_write_X509_CRL
	//��PEM��ʽ�ļ��ж�ȡCRL
	//���ú���PEM_read_X509_CRL()

    fwrite(buf,1,len,fp);
    fclose(fp);
    
    BIO_free(bp);
    X509_CRL_free(crl);
    free(buf);
    getchar();
    
    return 0;


//���CRL��չ��nidΪҪ��ӵ���չ��ʶ��valueΪ����ӵľ�����չ���
//�ڲ����ݽṹ��ַ��crit�����Ƿ�Ϊ�ؼ�����վ��flags�������ֲ�����
//X509_CRL_add1_ext_i2d

//�����չ�ָ����ջλ�ã��˺�������X509v3_add_ext
//X509_CRL_add_ext

//CRL�Ƚϣ��˺�������X509_NAME_cmp��ֵ�Ƚϰ䷢�ߵ������Ƿ���ͬ
//X509_CRL_cmp

//ɾ��CRL��չ���ջ�е�ĳһ�locָ����ɾ�����ڶ�ջ�е�λ��
//X509_CRL_delete_ext

//CRLժҪ,������ ��X509_CRL����ժҪ��typeָ��ժҪ�㷨��ժҪ��������md��
//X509_CRL_digest

//CRL���ݿ������˺���ͨ������ʵ�֡�
//X509_CRL_dup

//CRL�л�ȡ��չ��˺������ڻ�ȡcrl���ƶ���չ����ڲ����ݽṹ��
//����ֵΪ�������չ�����ݽṹ��ַ��nidΪ��չ��ʾ��
//��������X509V3_get_d2i����
//X509_CRL_get_d2i

//��ȡ��չ�������ջ�е�λ�ã�nidΪ��չ���ʶ��lastposΪ������ʼλ�á�
//X509_CRL_get_ext_by_NID

//��ȡ��չ�������ջ�е�λ��
//X509_CRL_get_ext_by_OBJ

//��ȡcrl����չ��ĸ���
//X509_CRL_get_ext_by_count

//��֤CRL.EVP_PKEY�ṹ����Ҫ������Կ
//X509_CRL_verify
}


/**
 * ���裺
 *      1����ʼ������
 *      a.�½�֤��洢��X509_STORE_new()
 *      b.�½�֤��У��������X509_STORE_CTX_new()
 *      
 *      2�������֤��
 *      a.��ȡCA֤�飬��DER�����ʽ��ΪX509�ṹd2i_X509()
 *      b.��CA֤�鵼��֤��洢��X509_STORE_add_cert()
 *      
 *      3������ҪУ���֤��test
 *      a.��ȡ֤��test����DER�����ʽ��ΪX509�ṹd2i_X509()
 *      b.��֤��У�������ĳ�ʼ��֤��test,X509_STORE_CTX_init()
 *      c.У��X509_verify_cert
 */

#define CERT_PATH "/root/workspace/caroot"
#define ROOT_CERT "ca.crt"
#define WIN71H "client.crt"
#define WIN71Y "win71y.cer"

#define GET_DEFAULT_CA_CERT(str) sprintf(str, "%s/%s", CERT_PATH, ROOT_CERT)
#define GET_CUSTOM_CERT(str, path, name) sprintf(str, "%s/%s", path, name)

#define MAX_LEGTH 4096


int32 rpc_verify_cert() {

 	OpenSSL_add_all_algorithms();
	
	int ret;
    char cert[MAX_LEGTH];

    unsigned char user_der[MAX_LEGTH];
    unsigned long user_der_len;
    X509 *user = NULL;

    unsigned char ca_der[MAX_LEGTH];
    unsigned long ca_der_len;
    X509 *ca = NULL;

    X509_STORE *ca_store = NULL;
    X509_STORE_CTX *ctx = NULL;
    STACK_OF(X509) *ca_stack = NULL;

    /* x509��ʼ�� */
    ca_store = X509_STORE_new();
    ctx = X509_STORE_CTX_new();

    /* root ca*/
    GET_DEFAULT_CA_CERT(cert);
    /* ���ļ��ж�ȡ */
    my_load_cert(ca_der, &ca_der_len, cert, MAX_LEGTH);
    /* DER����תX509�ṹ */
    ca = der_to_x509(ca_der, ca_der_len);
    /* ����֤��洢�� */
    ret = X509_STORE_add_cert(ca_store, ca);
    if ( ret != 1 )
    {
        fprintf(stderr, "X509_STORE_add_cert fail, ret = %d\n", ret);
        goto EXIT;
    }

    /* ��ҪУ���֤�� */
    GET_CUSTOM_CERT(cert, CERT_PATH, WIN71H);
    my_load_cert(user_der, &user_der_len, cert, MAX_LEGTH);
    user = der_to_x509(user_der, user_der_len);

    ret = X509_STORE_CTX_init(ctx, ca_store, user, ca_stack);
    if ( ret != 1 )
    {
        fprintf(stderr, "X509_STORE_CTX_init fail, ret = %d\n", ret);
        goto EXIT;
    }

    //openssl-1.0.1c/crypto/x509/x509_vfy.h
    ret = X509_verify_cert(ctx);
    if ( ret != 1 )
    {
    #if 0
        fprintf(stderr,
			"X509_verify_cert fail, ret = %d, error id = %d, %s\n",
                ret, 
                ctx->error, 
                X509_verify_cert_error_string(ctx->error));
	#endif
        goto EXIT;
    }
EXIT:
    X509_free(user);
    X509_free(ca);

    X509_STORE_CTX_cleanup(ctx);
    X509_STORE_CTX_free(ctx);

    X509_STORE_free(ca_store);

    return ret == 1 ? 0 : -1;
}
#endif
