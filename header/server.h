#pragma once

// c++ headers
#include <iostream>
#include <queue>

// networking headers
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>


#ifndef _SERVER_H_
#define _SERVER_H_

namespace server
{  
    class ServerBase
    {
    protected:
        int m_listenfd;
        struct sockaddr_in m_servaddr, m_cliaddr;
        socklen_t m_servlen, m_clilen;
        static void* handle_client(void* arg);

    public:
        ServerBase();
        ~ServerBase();
        virtual void startServer(char *port, int num_thread);

    protected:
        char* gethostip();
    };
    
} // namespace server

#endif