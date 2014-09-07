/* Derived from http://www.binarytides.com/get-local-ip-c-linux/ by Silver Moon */

/*
 * Find local ip used as source ip in ip packets.
 * Use getsockname and a udp connection
 */

#include<stdio.h> //printf
#include<string.h>    //memset
#include<errno.h> //errno
#include<sys/socket.h>    //socket
#include<netinet/in.h> //sockaddr_in
#include<arpa/inet.h> //getsockname
#include<unistd.h>    //close

#define ERROR_STRING "Unable to guess local IP address"

const char *local_ip (void)
{
    const char* google_dns_server = "8.8.8.8", *ip = NULL;
    int dns_port = 53;
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    struct sockaddr_in serv;
    static char buffer[20];
    int sock = socket ( AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror(ERROR_STRING);
        return NULL;
    }

    memset( &serv, 0, sizeof(serv) );
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr( google_dns_server );
    serv.sin_port = htons( dns_port );

    if (0 != connect( sock , (const struct sockaddr*) &serv , sizeof(serv) )
            || 0 != getsockname(sock, (struct sockaddr*) &name, &namelen)
            || !inet_ntop(AF_INET, &name.sin_addr, buffer, sizeof(buffer) ))
        goto err;

    ip = buffer;

err:
    if (!ip)
        printf("%s: %s\n" , ERROR_STRING, strerror(errno));
    if (sock > 0)
        close(sock);

    return ip;
}
