/*
 * =====================================================================================
 *
 *       Filename:  socket.cc
 *
 *    Description:  sockets for api
 *
 *        Version:  1.0
 *        Created:  2015年11月25日 17时25分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (chenhao), 
 *        Company:  
 *
 * =====================================================================================
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "base_code.h"
extern Loger log;
#define _ log._

int Socket(const char *host, int clientPort)
{
    int sock;
    unsigned long inaddr;
    struct sockaddr_in ad;
    struct hostent *hp;
    
    memset(&ad, 0, sizeof(ad));
    ad.sin_family = AF_INET;

    inaddr = inet_addr(host);
    if (inaddr != INADDR_NONE)
        memcpy(&ad.sin_addr, &inaddr, sizeof(inaddr));
    else
    {
        hp = gethostbyname(host);
        if (hp == NULL)
            return -1;
        memcpy(&ad.sin_addr, hp->h_addr, hp->h_length);
    }
    ad.sin_port = htons(clientPort);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
	{
		_(NO_LOCK, "socket generate err");
        return sock;
	}
    if (connect(sock, (struct sockaddr *)&ad, sizeof(ad)) < 0)
	{
		_(NO_LOCK, "socket connect err cause [%s]", strerror(errno));
        return -1;
	}

	_(NO_LOCK, "socket connection [%s]port[%d]ok", host, clientPort);
    return sock;
}

