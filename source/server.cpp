#include "../header/threadpool.h"
#include "../header/server.h"
#include <fcntl.h>

namespace server
{
    ServerBase::ServerBase()
    {
        std::cout << "In ServerBase()\n";
    }

    ServerBase::~ServerBase()
    {
        std::cout << "In ~ServerBase()\n";
    }

    void ServerBase::startServer(char *port, int num_thread)
    {   
        // std::cout << "Enter startServer()\n";
        /* create the threadPool and assign the handle function */
        threadpool::ThreadPool pool;
        pool.m_task = handle_client;
        pool.create_pool(num_thread);

        /* start the server */
        if((m_listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("[socket error]:");
            exit(-1);
        }

        int enable = 1;
        if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        {
            perror("[setsockopt(SO_REUSEADDR) failed] : ");
            exit(-1);    
        }

        // char* ip = gethostip();

        bzero(&m_servaddr, sizeof(m_servaddr));
        m_servaddr.sin_addr.s_addr = INADDR_ANY; //htonl(atoi(ip));
        m_servaddr.sin_family = AF_INET;
        m_servaddr.sin_port = htons(atoi(port));

        m_servlen = sizeof(m_servaddr);

        if(bind(m_listenfd, (sockaddr*)&m_servaddr, m_servlen) < 0)
        {
            perror("[bind error]:");
            exit(-1);
        }

        if(listen(m_listenfd, 100) < 0)
        {
            perror("[listen error]:");
            exit(-1);
        }

        int connfd;
        while(1)
        {
            std::cout << "Waiting for connections\n";
            m_clilen = sizeof(m_cliaddr);
            if((connfd = accept(m_listenfd, (sockaddr*)&m_cliaddr, &m_clilen)) < 0)
            {
                if(errno == EINTR)
                    continue;
                
                else
                {
                    perror("[accept error]:");
                    exit(-1);
                }
            }
            
            /* set connfd to nonblocking */
            
            int flags;
            if((flags = fcntl(connfd, F_GETFL, 0)) < 0)
            {
                perror("[fcntl  (F_GETFL) error] : ");
                exit(-1);
            }
            flags|=O_NONBLOCK;
            if(fcntl(connfd, F_SETFL, flags) < 0)
            {
                perror("[fcntl (F_SETFL) error] : ");
                exit(-1);
            }
            std::cout << connfd << " non-blocking done\n";
            

            pool.deligate_job(connfd);
            // std::cout << "in startServer, after deligate_job()\n";
        }

        pool.destroy_pool();

    }


    void* ServerBase::handle_client(void* arg)
    {
        // std::cout << "Enter handle_client()\n";
        threadpool::thread *curr_thread = (threadpool::thread *)arg;
        threadpool::ThreadPool *pool = curr_thread->pool;


        for(;;)
        {
            pool->lock_mutex(curr_thread->m_thread_mutex);
            
            while (curr_thread->m_fd_queue.empty())
                pool->wait_cond(curr_thread->m_thread_cond, curr_thread->m_thread_mutex);
            
            
            std::cout << "connfd = " << curr_thread->m_fd_queue.front() << "thread_id = " << curr_thread->id << "\n";
        
            if(curr_thread->m_fd_queue.empty())
            {
                perror("[Something wrong. Wrong thread is signalled]");
                exit(-1);
            }

            int *connfd = new int;
            *connfd = curr_thread->m_fd_queue.front();
            curr_thread->m_fd_queue.pop();
            std::cout << curr_thread->id << " has released mutex\n";

            pool->unlock_mutex(curr_thread->m_thread_mutex);

            /* Select starts from here */
            int nready;
            std::cout << " blocked in select\n";
            curr_thread->m_rset = curr_thread->m_allset;
            struct timeval l_time;
            l_time.tv_sec = 5;        //2 sec.
            l_time.tv_usec = 60000;   //0.06 sec.


            while((nready = select( curr_thread->m_maxfd+1 , &curr_thread->m_rset , NULL , NULL , &l_time )) <= 0)
            {

                // FD_ZERO(&curr_thread->m_rset);
                // FD_SET(*connfd, &curr_thread->m_rset);
                // FD_SET(STDIN_FILENO, &curr_thread->m_rset);
                std::cout << "nik -2\n";

                if(nready == 0){ // timeout.
                    std::cout << "nik -1\n";
                    l_time.tv_sec = 5;        //2 sec.
                    l_time.tv_usec = 60000;   //0.06 sec.
                    // FD_ZERO(&curr_thread->m_rset);
                    pool->reSet(curr_thread);
                    // if(FD_ISSET(*connfd, &curr_thread->m_rset))
                    // {
                    //     std::cout << "connfd was set.\n";
                    //     // FD_SET(*connfd, &curr_thread->m_rset);
                    // }
                    // else
                    //     FD_SET(*connfd, &curr_thread->m_rset);
                    continue;
                }
                else
                {
                    std::cout << "nik0\n";
                    perror("[select error] : ");
                    exit(-1);
                }
            }

            
            // if((nready = select( curr_thread->m_maxfd+1 , &curr_thread->m_rset , NULL , NULL , NULL )) < 0)
            // {
            //     perror("[select error] : ");
            //     exit(-1);
            // }
            

            std::cout << " got out of select\n";

            // if(l_time.tv_sec == 0 && l_time.tv_usec == 0)
            //     continue ; // no request came from the current fd, refresh the values of m_maxfd and m_rset.

            int i, l_connfd, n;
            char buff[1024];

            std::cout << "nik1\n";
            for(i=0 ; i<=curr_thread->m_maxi ; i++)
            {
                std::cout << "nik2\n";
                if((l_connfd = curr_thread->m_clientfd_array[i]) < 0)
                    continue;

                std::cout << "nik3\n";
                if(FD_ISSET(l_connfd, &curr_thread->m_rset))
                {
                    FD_CLR(l_connfd, &curr_thread->m_allset);
                    std::cout << "nik4\n";
                    std::cout << "Before recv\n";
                    if((n = recv(l_connfd, buff, 1024, 0))<=0)
                    {   
                        std::cout << "nik1\n";
                        if(n<0)
                        {
                            if(errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                continue;
                            }
                            else
                            {
                                perror("[recv error] : ");
                                exit(-1);
                            }
                        }
                        /* connection closed by client. */
                        pool->lock_mutex(curr_thread->m_thread_mutex);

                        std::cout << "in recv-[if], closing l_connfd\n";
                        close(l_connfd);
                        // FD_CLR(l_connfd, &curr_thread->m_allset);
                        curr_thread->m_clientfd_array[i] = -1;
                        pool->unlock_mutex(curr_thread->m_thread_mutex);
                    }
                    else{
                        // std::cout << "before send\n";
                        send(l_connfd, buff, n, 0);
                        // std::cout << "after send\n";

                        pool->lock_mutex(curr_thread->m_thread_mutex);

                        std::cout << "in send-[if] closing l_connfd\n";
                        close(l_connfd);
                        
                        curr_thread->m_clientfd_array[i] = -1;

                        pool->unlock_mutex(curr_thread->m_thread_mutex);
                    }
                    
                    if(--nready <= 0)
                        break;          /* no more readable descriptors. */
                }
            }
            // curr_thread->m_rset = curr_thread->m_allset;
            // std::cout << "Exit handle_client()\n";
        }
	    return NULL;
    }
    


    char* ServerBase::gethostip()
    {
        char hostbuffer[256]; 
        char *IPbuffer; 
        struct hostent *host_entry; 
        int hostname; 
    
        if((hostname = gethostname(hostbuffer, sizeof(hostbuffer)) ) <0)
        { 
            perror("gethostname"); 
            exit(1); 
        }
    
        if((host_entry = gethostbyname(hostbuffer)) == NULL) 
        { 
            perror("gethostbyname"); 
            exit(1); 
        }  
        

        IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])); 
        if(NULL == IPbuffer) 
        { 
            perror("inet_ntoa"); 
            exit(1); 
        } 
        return IPbuffer;
    }

} // namespace server
