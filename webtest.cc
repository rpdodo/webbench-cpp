/*
 * =====================================================================================
 *
 *       Filename:  webtest.cc
 *
 *    Description:  webtest
 *
 *        Version:  1.0
 *        Created:  2015年11月25日 16时10分19秒
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  (chenhao)
 *        Company:  
 *
 * =====================================================================================
 */

//-------------------------------------------------------------------------
// includes
//-------------------------------------------------------------------------
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <assert.h>
#include <getopt.h>
#include <map>
#include "base_code.h"

//-------------------------------------------------------------------------
// Loger
//-------------------------------------------------------------------------
Loger log("webtest.log");
#define _ log._

//-------------------------------------------------------------------------
// Macro
//-------------------------------------------------------------------------
#define LINE_END "\r\n"
#define BUF_LEN 8192
enum {GET, HEAD, OPTIONS, TRACE};
static std::map<std::string, int> mMthod;

//-------------------------------------------------------------------------
// struct
//-------------------------------------------------------------------------
struct Options{
	// clients Number(fork) for numeric
	// each client Count(sockets) for numeric
	// send Timeperiod(seconds) for numberic
	// Force for send without socket send buffer and not recv data
	int iClients ;
	int iCount ;
	int iTimeperiod ;
	bool bForce ;
	int iMethod ;

	char csHost[1024];
	int iPort;

	Options():iClients(1),iCount(10), iTimeperiod(30), 
	bForce(false), iMethod(GET), iPort(80) {memset(csHost, 0x00, 1024);}
};

//-------------------------------------------------------------------------
// api
//-------------------------------------------------------------------------
int setOptions(int argc, char** argv, Options& opts);
int Socket(const char *host, int clientPort);
void BulidRequest(const Options& opts, std::string& sRequrest);
void PrintUsage();
//-------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	_(NO_LOCK, "webtest begin");
	if (argc == 1)
	{
		PrintUsage();
		return 0;
	}

	Options opts;
	if (setOptions(argc, argv, opts))
	{
		_(NO_LOCK, "setOptions err");
		return 0;
	}

	// test target host connection
	int sock = Socket(opts.csHost, opts.iPort);
	if (sock <= 0){
		fprintf(stderr, "Target Host Port can not open\n");
		_(NO_LOCK, "Target Host Port can not open");
		return -1;
	}
	close(sock);

	// build request
	std::string sRequrest;
	BulidRequest(opts, sRequrest);
	//_(NO_LOCK, "[%s]", sRequrest.c_str());
	
	//fork clients
	pid_t pid;
	for(int i = 0; i < opts.iClients; i++)
	{
		pid = fork();
		if (pid <= 0)
		{
			break;
		}
	}

	// common code for child and parent
	assert(pid >= 0);

	if (pid == 0) // child
	{
		int n;
		int socks;

		for(int times = 1; times <= opts.iCount; times++)
		{
			int socks= Socket(opts.csHost, opts.iPort);
			if(socks <= 0)
			{
				_(NO_LOCK, "Target host port unable to open");
				return -1;
			}

			//re-use socks
			char bReuseaddr = 0xff;
			setsockopt(socks, SOL_SOCKET, SO_REUSEADDR,(const char*)&bReuseaddr, sizeof(bReuseaddr));

			if (opts.bForce) // set socket send buff size 0
			{
				int iZeroBuf = 0;
				setsockopt(socks, SOL_SOCKET, SO_SNDBUF, (char*)&iZeroBuf, sizeof(iZeroBuf));
			}

			//send !!!
			n = send(socks, sRequrest.c_str(), sRequrest.size(), 0);
			if (n <= 0)
			{
				_(NO_LOCK, "send err");
			}

			_(NO_LOCK, "send len [%d]send times [%d]", n, times);

			if (!opts.bForce) // recv if not force
			{
				//recv !!!
				char rBuf[BUF_LEN] = "";
				int recvtimes = 1;
				while(n > 0)
				{
					memset(rBuf, 0x00, BUF_LEN);
					n = recv(socks, rBuf, BUF_LEN, 0);
					if (n <= 0)
					{
						_(NO_LOCK, "--rBuf len[%d] recv times[%d] end" ,n, recvtimes++);
					}else{
						//_(NO_LOCK, "rBuf[%s]len[%d]times[%d]",rBuf , n, recvtimes++);
						fprintf(stdout, "rBuf[%s]len[%d]times[%d]",rBuf , n, recvtimes++);
						_(NO_LOCK, "--rBuf len[%d] recv times[%d]", n, recvtimes++);
					}
				}
			}
			close(socks);
		}
	}

	if (pid > 0)// clean bodys
	{
		pid_t wpid;
		//while( (wpid = waitpid(-1, NULL, WNOHANG)) > 0 );
		while( (wpid = waitpid(-1, NULL, 0)) > 0 );

		_(NO_LOCK, "webtest end");
	}

	return 0;
}


//-------------------------------------------------------------------------
// local function
//-------------------------------------------------------------------------
void BulidRequest(const Options& opts, std::string& sRequrest)
{
	sRequrest = "GET / HTTP/1.1"LINE_END;
	sRequrest += ("Host: "+ static_cast<std::string>(opts.csHost) + ":" + toString(opts.iPort) + LINE_END);
	sRequrest += ("Connection: keep-alive"LINE_END);
	sRequrest += ("Accept: */*"LINE_END);
	sRequrest += ("User-Agent: Webtest"LINE_END);
	sRequrest += ("Pragma: no-cache"LINE_END);
	sRequrest += LINE_END;
}

void PrintUsage()
{
	std::cout<< "Usage: webtest [options]... host:port\n"; 
	std::cout<< "		-n <client number> 	how many clients\n"; 
	std::cout<< "		-c <socket count> how many socket each client\n"; 
	std::cout<< "		-t <time> how many seconds\n"; 
	std::cout<< "		-f force sending without socket send buffer and do not recv data\n"; 
}

int setOptions(int argc, char** argv, Options& opts)
{
	mMthod["GET"]=GET;
	mMthod["get"]=GET;
	mMthod["HEAD"]=HEAD;
	mMthod["head"]=HEAD;
	mMthod["TRACE"]=TRACE;
	mMthod["trace"]=TRACE;
	mMthod["OPTIONS"]=OPTIONS;
	mMthod["options"]=OPTIONS;

	int opt = 0;
	while( (opt = getopt(argc, argv, "m:n:c:t:f?h")) != EOF )
	{
		switch(opt){
			case 'n': opts.iClients = atoi(optarg); break;
			case 'c': opts.iCount = atoi(optarg); break;
			case 't': opts.iTimeperiod= atoi(optarg); break;
			case 'f': opts.bForce= true; break;
			case 'm':
					  opts.iMethod = mMthod[optarg];
					  break;
			case ':':
			case '?':
			case 'h': PrintUsage(); return -1;
		}
	}

	if (optind == argc)
	{
		PrintUsage();
		return -1;
	}

	if ( argc != 0 )
	memcpy(opts.csHost ,argv[optind], strlen(argv[optind]));
	char* p = opts.csHost;
	while (*p++ != ':') 
	{
		if (*p == ':')
			{*p = '\0';break;}
	}
	opts.iPort = atoi(++p);

	std::cout<< "clients ["<< opts.iClients <<"] \n";
	std::cout<< "sockets each client ["<< opts.iCount<<"] \n";
	std::cout<< "Timeperiod ["<< opts.iTimeperiod<<"] \n";
	std::cout<< "force flag ["<< opts.bForce<<"] \n";
	std::cout<< "Method ["<< opts.iMethod<<"] \n";
	std::cout<< "host ["<< opts.csHost<<"] \n";
	std::cout<< "port ["<< opts.iPort<<"] \n"; 

	return 0;
}
