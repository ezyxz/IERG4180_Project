#include "UrlParse.h"

int  parse_domain_dir(char *url,MY_URL_RESULT *result)
{
	char *p;
	char *token = NULL;
	char buf[MAX_URL_LEN] = {0};
	char dir_str[MAX_PATH_FILE_LEN*2] = {0};
	char svr_dir[MAX_COMM_NAME_LEN] = {0}; 	
	char ip_port[MAX_COMM_NAME_LEN*2] = {0};//域名:port
	char domain[INET_DOMAINSTRLEN] = {0};
	char port_s[MAX_PORT_LEN] = {0};

	char user_pass[MAX_USER_LEN + MAX_PASS_LEN] = {0};
	char user[MAX_USER_LEN] = {0};
	char pass[MAX_PASS_LEN] = {0};

	int test;

	p = buf;

	//strtok后 buf中内容会被修改，所以先把url内容拷贝出来
	strncpy(buf,url,sizeof(buf));
	
	//"baidu.com:8080/dir/index.html"

	token = strtok(buf, "@");
	if(token != NULL) {
#if _URL_DEBUG
		printf("token=%s\n",token);
#endif
		if(strlen(token) != strlen(url)) //找不到@后，返回值指向头
		{
			if(strlen(user_pass) == 0) {
				strncpy(user_pass, token,sizeof(user_pass));
				p += strlen(user_pass)+1; //指向@后字符串
			} 
		}
	}
#if _URL_DEBUG
	printf("user_pass=%s\n",user_pass);
#endif	

	//提取用户名密码
	if(strlen(user_pass) > 0)
	{
		token = strtok(user_pass, ":");
		 while(token != NULL) {
			if(strlen(user) == 0) {
				strncpy(user, token,sizeof(user));
				if((strlen(user)==0)||(strlen(user)>MAX_USER_LEN))
				{
					printf("%s() %d\n",__func__,__LINE__);
					return URL_ERROR;
				}
			} else{
				strcat(pass,token);
			}
			token = strtok(NULL, "/");
		}
	}
	
//192.168.1.100:990/LE2.0    192.168.20.10/dir1/dir2/index.html   baidu.com/down   baidu.com
//body: baidu.com:8080/dir/index.html

	//获取域名+端口号
	 token = strtok(p, "/");
	 while(token != NULL) {
		if(strlen(ip_port) == 0) {
			strncpy(ip_port, token,sizeof(ip_port));
			if((strlen(ip_port)==0)||(strlen(ip_port)>INET_DOMAINSTRLEN + MAX_PORT_LEN + 1))
			{
				printf("%s() %d\n",__func__,__LINE__);
				return URL_ERROR;
			}
		} else{
			strcat(dir_str,"/");
			strcat(dir_str,token);
		}

		token = strtok(NULL, "/");
	}
#if _URL_DEBUG
	printf("ip_port=%s\n",ip_port);
	printf("svr_dir:%s\n\n",dir_str);
#endif	 

	
	token = strtok(ip_port, ":");
	 while(token != NULL) {
		if(strlen(domain) == 0) {
			strncpy(domain, token,sizeof(domain));
			if((strlen(domain)==0)||(strlen(domain)>INET_DOMAINSTRLEN))
			{
				printf("%s() %d invalid daomain length\n",__func__,__LINE__);
				return URL_ERROR;
			}
		}else if(strlen(port_s) == 0){
			strncpy(port_s, token,sizeof(port_s));
			if((strlen(port_s)==0)||(strlen(port_s)>MAX_PORT_LEN))
			{
				printf("%s() %d invalid port length\n",__func__,__LINE__);
				return URL_ERROR;
			}
		}
		token = strtok(NULL, ":");
	} 
	if(strlen(domain)==0)
	{
		printf("%s() %d there is no domain\n",__func__,__LINE__);
		return URL_ERROR;
	}
	if(strlen(port_s) > 0)
	{
		result->port = atoi(port_s);		
	}

	if(strlen(dir_str)==0){
		//baidu.com  没有路径 用默认/
		strcat(dir_str,"/");
	}

	strncpy(result->user,user,sizeof(result->user));
	strncpy(result->pass,pass,sizeof(result->pass));
	strncpy(result->domain,domain,sizeof(result->domain));
	strncpy(result->svr_dir,dir_str,sizeof(svr_dir));

#if _URL_DEBUG
	
	printf("user:%s\n",result->user);
	printf("pass:%s\n",result->pass);
	printf("port:%d\n",result->port);
	printf("domain:%s\n",result->domain);
	printf("svr_dir:%s\n\n\n",result->svr_dir);
#endif
	return URL_OK;
}



void remove_quotation_mark(char *input)
{
    int i=0;
    char tmp_buf[MAX_URL_LEN];
    char *tmp_ptr = input;

    while(*tmp_ptr != '\0') {
	if(*tmp_ptr != '"') {
	    tmp_buf[i] = *tmp_ptr;
	    i++;
	}
	tmp_ptr++;
    }
    tmp_buf[i] = '\0';
    strcpy(input, tmp_buf);
}


/*
return code
-1 : invalid param  0:domain   1 :ipv4 
*/
 int check_is_ipv4(char *domain)
 {
	struct in_addr s;  
	char IPdotdec[20] = {0}; 

	if((strlen(domain) == 0) || (strlen(domain)>INET_DOMAINSTRLEN))
	{
		printf("invalid domain length!\n");	
		return -1;
	}
	if(inet_pton(AF_INET, domain, (void *)&s)==1)
	{
		inet_ntop(AF_INET, (void *)&s, IPdotdec, 16);
		//printf("fota_check_is_ipv4 %s\n", IPdotdec);
		return 1;
	}else{
		//printf("may be domain\n");	
		return -1;
	}
 }


int parse_url(char *raw_url,MY_URL_RESULT *result)
{
	int i ;
	int ret;
	char *p = NULL;
	char out_buf[MAX_URL_LEN];//url
	char body[MAX_URL_LEN];



	strcpy(out_buf, "");
	strcpy(body, "");		


	strncpy(out_buf, raw_url,sizeof(out_buf));
	p = out_buf;

	if(strstr(out_buf, "\"") != NULL) 
	{	               
		remove_quotation_mark(out_buf);	            
	}

	for(i=0;i<ARRAY_SIZE(g_pro_port1);i++)
	{	
		if(strncasecmp(g_pro_port1[i].pro_s,p,strlen(g_pro_port1[i].pro_s))==0)
		{	
			p += strlen(g_pro_port1[i].pro_s);
			strncpy(body, p,sizeof(body));
			result->port = g_pro_port1[i].port;
			break;
		}
	}
	if(i == ARRAY_SIZE(g_pro_port1))
	{
		printf("%s: Error: invalid protocol %s\n", __FUNCTION__,out_buf);
		return URL_ERROR;	
	}

#if _URL_DEBUG
	printf("%s():\nbody: %s\n", __FUNCTION__,body);
#endif

	if(strstr(body, "\"") != NULL) 
	{	               
		remove_quotation_mark(body);	            
	}

//解析域名、端口号、文件目录
	ret = parse_domain_dir(body,result);
	if(ret == URL_ERROR)
	{
		ret = printf("parse_domain_dir() err\n");
		return URL_ERROR;
	}	
	return ret;
}



int dns_resoulve(char *svr_ip,const char *domain)
{
    char   **pptr;
    struct hostent *hptr;
    char   str[MAX_IP_STR_LEN];

    if((hptr = gethostbyname(domain)) == NULL)
    {
        printf(" gethostbyname error for host:%s\n", domain);
        return URL_ERROR;
    }
#if _URL_DEBUG

    printf("official hostname:%s\n",hptr->h_name);
    for(pptr = hptr->h_aliases; *pptr != NULL; pptr++)
        printf(" alias:%s\n",*pptr);
#endif
    switch(hptr->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            pptr=hptr->h_addr_list;
		#if _URL_DEBUG
            for(; *pptr!=NULL; pptr++)
            {
                printf(" address:%s\n",inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
            }
            printf(" first address: %s\n",inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str)));
		#else	
			inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
		#endif	
			strncpy(svr_ip,str,MAX_IP_STR_LEN);
		
        break;
        default:
            printf("unknown address type\n");
        break;
    }

    return URL_OK;
}


