// https://gitee.com/yikoulinux/urls
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


#define _URL_DEBUG 0

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


#define HEAD_FTP_P "ftp://"
#define HEAD_FTPS_P "ftps://"   
#define HEAD_FTPES_P "ftpes://"
#define HEAD_HTTP_P "http://"
#define HEAD_HTTPS_P "https://"


#define PORT_FTP  21
#define PORT_FTPS_I  990    //implicit
#define PORT_FTPS_E  21     //explicit
#define PORT_HTTP 80
#define PORT_HTTPS 443

#define URL_ERROR -1
#define URL_OK 0


struct pro_port{
	char pro_s[32];
	unsigned short port;
};

struct pro_port g_pro_port1[]={
	{HEAD_FTP_P,PORT_FTP},
	{HEAD_FTPS_P,PORT_FTPS_I},	
	{HEAD_FTPES_P,PORT_FTPS_E},	
	{HEAD_HTTP_P,PORT_HTTP},	
	{HEAD_HTTPS_P,PORT_HTTPS},
};

#define MAX_COMM_NAME_LEN 128

#define MAX_URL_LEN 1024
#define INET_ADDRSTRLEN 16
#define INET_DOMAINSTRLEN 128
#define MAX_PORT_LEN 6
#define MAX_PATH_FILE_LEN 256
#define MAX_IP_STR_LEN 32

#define MAX_USER_LEN 32
#define MAX_PASS_LEN 32


typedef struct MY_URL_RESULT
{
	char user[MAX_USER_LEN];
	char pass[MAX_PASS_LEN];
	char domain[INET_DOMAINSTRLEN];//域名
	char svr_dir[MAX_PATH_FILE_LEN]; //文件路径
	char svr_ip[MAX_IP_STR_LEN];
	int port;
}MY_URL_RESULT;


 #ifndef URL_PARSE
 #define  URL_PARSE
int  parse_domain_dir(char *url,MY_URL_RESULT *result);
void remove_quotation_mark(char *input);

int parse_url(char *raw_url,MY_URL_RESULT *result);
int dns_resoulve(char *svr_ip,const char *domain);
int check_is_ipv4(char *domain);
void remove_quotation_mark(char *input);
 #endif