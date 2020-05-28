
// c++ headers
#include <iostream>
#include <queue>
#include <vector>


// threading headers and etc
#include <sys/prctl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <pthread.h>
#include <string.h>


#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

typedef int connection_fd_t;

namespace threadpool
{
    class ThreadPool;

    typedef struct thread
    {
        int id;
        pthread_t m_thread;
        std::queue<connection_fd_t> m_fd_queue;
        pthread_mutex_t m_thread_mutex;
        pthread_cond_t m_thread_cond;
        ThreadPool *pool;           // pointer to the ThreadPool. Used to access mutex n cond functions.

        fd_set m_allset, m_rset;
        int m_clientfd_array[FD_SETSIZE];
        int m_maxfd, m_maxi;

        thread();
    } thread; 


    class ThreadPool
    {
        protected:
            int m_num_threads;
            thread* m_thread_list;
            static int m_thread_iterator;
            void* m_arg;
            

        public:
            ThreadPool();
            void* (*m_task)(void* arg);
            pthread_mutex_t m_pool_mutex;
            pthread_cond_t m_pool_cond;

            void create_pool(int n);
            void deligate_job(int connfd);
            void destroy_pool();

            /* We have API for this, but i want to access m_cond outside class, without making it public. */
            void wait_cond(pthread_cond_t &cond, pthread_mutex_t &mutex);
            void wake_cond(pthread_cond_t &cond);
            void lock_mutex(pthread_mutex_t &mutex);
            void unlock_mutex(pthread_mutex_t &mutex);

            void reSet(thread *curr_thread);
            /*
                typedef struct job
                {
                    void* (function)(void* arg);
                    void* arg;
                    struct job* next;
                } job;
                job* job_queue;
                job *front, *rear;
            */

    }; // class ThreadPool
    

} // namespace threadpool

#endif
