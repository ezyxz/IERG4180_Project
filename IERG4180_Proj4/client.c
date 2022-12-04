// -lssl -lcrypto/
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <stdlib.h>
#include <stdio.h>

# include <sys/types.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <sys/fcntl.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <errno.h>
#include "sys/time.h"
char* rhost = "127.0.0.1";
char* lhost = "127.0.0.1";
char dest_url[8192];
X509 *cert = NULL;
X509_NAME *certname = NULL;
const SSL_METHOD *method;
SSL_CTX *ctx;
SSL *ssl;
int server = 0;
int ret, i;
char *ptr = NULL;

int create_socket();
void ShowCerts(SSL* ssl);
int main(int argc, char **argv){

    server = create_socket();
    if (server != 0)
        printf("Successfully made the TCP connection to: %s.\n", rhost);
    
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    if (SSL_library_init() < 0)
        printf("Could not initialize the OpenSSL library !\n");

    method = TLS_method();
    if ((ctx = SSL_CTX_new(method)) == NULL)
        printf("Unable to create a new SSL context structure.\n");
    
    // ret = SSL_CTX_load_verify_file(ctx, "domain.crt");
    // if (ret == 1){
    //     printf("crt added to cert store.\n");
    // }
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server);
    SSL_set_tlsext_host_name(ssl, rhost); // Enable Server-Name-Indication
    if (SSL_connect(ssl) != 1)
        printf("Error: Could not build a SSL session to: %s.\n", rhost);
    else
        printf("Successfully enabled SSL/TLS session to: %s.\n", rhost);
    

    ShowCerts(ssl);
    cert = SSL_get_peer_certificate(ssl);
    if (cert == NULL)
        printf("Error: Could not get a certificate from: %s.\n", rhost);
    else
        printf("Retrieved the server's certificate from: %s.\n", rhost);
    
    ret = SSL_get_verify_result(ssl);
        
    if (ret != X509_V_OK)
        printf("Warning: Validation failed for certificate from: %s.\n", rhost);
    else
        printf("Successfully validated the server's certificate from: %s.\n",rhost);


    // ret = X509_check_host(cert, rhost, strlen(rhost), 0, &ptr);
    // if (ret == 1) {
    //     printf("Successfully validated the server's hostname matched to: %s.\n", ptr);
    //     OPENSSL_free(ptr); ptr = NULL;
    // }
    // else if (ret == 0)
    //     printf("Server's hostname validation failed: %s.\n", rhost);
    // else
    //     printf("Hostname validation internal error: %s.\n", rhost);

    // Send an HTTP GET request

    char request[8192];
    sprintf(request,
        "GET / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n", rhost);
    SSL_write(ssl, request, strlen(request));
    // Receive and print out the HTTP response
    printf("------------------ RESPONSE RECEIVED ---------------------\n");
    int len = 0;
    do {
        char buff[1536];
        len = SSL_read(ssl, buff, sizeof(buff));
        if (len > 0) 
            fwrite(buff, len, 1, stdout);
    } while (len > 0);
    printf("\n----------------------------------------------------------\n");
}

int create_socket() {
    int sockfd; int port;
    struct sockaddr_in dest_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    port = 4181;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(rhost);
    if (connect(sockfd, (struct sockaddr *) &dest_addr,
    sizeof(struct sockaddr)) == -1) {
        printf("Error: Cannot connect to host %s  on port %d.\n",rhost, port);
    }
    return sockfd;
}

void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}
