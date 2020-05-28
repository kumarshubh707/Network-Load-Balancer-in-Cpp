
/* 
    Assign the m_task,m_arg in main function, where object is created 
    
    *   like:
    *       threadpool::ThreadPool ob;
    *       ob.m_task = handle;
*/


#include "../header/threadpool.h"

namespace threadpool
{
    int ThreadPool::m_thread_iterator = 0;

    thread::thread()
    {
        std::cout << "In thread(), id =  "<< id << "\n";
        pthread_mutex_init(&m_thread_mutex, 0);
        pthread_cond_init(&m_thread_cond, 0);
        
        // set related.
        FD_ZERO(&m_allset);
        FD_ZERO(&m_rset);
        m_maxfd = 0; // for argument to select().
        m_maxi = 0; // for index in m_client_array[].
        
        for(int i=0 ; i<FD_SETSIZE ; i++)
            m_clientfd_array[i] = -1;
    }

    ThreadPool::ThreadPool()
    {
        pthread_mutex_init(&m_pool_mutex, 0);
        pthread_cond_init(&m_pool_cond, 0);

        m_task = NULL;
        m_arg = NULL;
    }
    
    /*
        *  If you want to create a thread that creates a pool, do this. Here, i realised that `struct thread` should hve its own 
        *  task() and arg. That is when we can actually make our code modular. 
        
        // thread new_thr;
        // pthread_create(&new_thr.m_thread, 0, create_pool, this);

        * If m_task is not yet assigned to a handle function.
        *   a) Wait for it. This thread goes to sleep, and is waken up in main function, [if this thread is sleeping..]
        *   b) Return from the program.
    */
    void ThreadPool::create_pool(int thread_count)
    {
        // std::cout << "Enter create_pool()\n";
        m_num_threads = thread_count;

        /* allocate dynamic memory to "thread_count" num of threads */
        m_thread_list = new threadpool::thread [thread_count];

        if (m_task == NULL)
        {
            // wait_cond(m_pool_cond, m_pool_mutex);
            perror("[task is not yet assigned to a function] : ");
            exit(-1);
        }

        /* create threads */
        for (int i = 0; i < thread_count; i++)
        {
            m_thread_list[i].id = i;
            m_thread_list[i].pool = this;
            m_arg = (void *)&m_thread_list[i];
            pthread_create(&m_thread_list[i].m_thread, 0, m_task, m_arg);   // inside m_task, there's a wait condition on `size of queue` (m_arg->fd_queue).
                                                                            // Its waked up in deligate_job().
            // pthread_detach(m_thread_list[i].m_thread);                      // detach the created thread. (necessary??)
        }

        std::cout << "Pool Created\n";
        // std::cout << "Exit create_pool()\n";
    }

    void ThreadPool::deligate_job(int connfd)
    {
        std::cout << "Enter deligate_job()\n";
        
        lock_mutex(this->m_pool_mutex);
        int current_iterator = m_thread_iterator++;    //this is not an atomic operation.
        unlock_mutex(this->m_pool_mutex);

        lock_mutex(m_thread_list[current_iterator].m_thread_mutex);

        std::cout << "m_thread_iterator = " << current_iterator <<"\n";
        
        if (m_thread_list == NULL)
        {
            perror("[thread_list is empty] : ");
            destroy_pool();
            exit(-1);
        }

        // m_thread_iterator++;
        if (m_thread_iterator == m_num_threads)
        {
            m_thread_iterator = 0;
        }

        // put this fd into the respective thread's clientfd array.
        int i;
        for(i=0 ; i<FD_SETSIZE ; i++)
        {
            if(m_thread_list[current_iterator].m_clientfd_array[i] < 0)
            {
                m_thread_list[current_iterator].m_clientfd_array[i] = connfd;
                break;
            }
        }
        if(i==FD_SETSIZE)
        {
            perror("[too many clients] : ");
            exit(-1);
        }

        FD_SET(connfd, &m_thread_list[current_iterator].m_allset);

        if(connfd > m_thread_list[current_iterator].m_maxfd)
            m_thread_list[current_iterator].m_maxfd = connfd;

        if(i > m_thread_list[current_iterator].m_maxi)
            m_thread_list[current_iterator].m_maxi = i;
        // handling thread's fd_set is over.
        
        m_thread_list[current_iterator].m_fd_queue.push(connfd);        

        wake_cond(m_thread_list[current_iterator].m_thread_cond); // wake up the corr thread.
        
        std::cout<<"done delegating "<< connfd <<"\n";
        unlock_mutex(m_thread_list[current_iterator].m_thread_mutex);
        // std::cout << "Exit deligate_job()\n";
    }

    void ThreadPool::reSet(thread *current_thread)
    {
        for(int i=0 ; i<=current_thread->m_maxi ; i++)
        {
            if(current_thread->m_clientfd_array[i] > 0)
                FD_SET(current_thread->m_clientfd_array[i], &current_thread->m_rset); 
        }
        
    }

    void ThreadPool::destroy_pool()
    {
        // close all the connfds (will exit(0) take care of this?).
        // destroy all the `thread` mutexes and conditions.
        // deallocate memory given to m_thread_list.
        // destroy the `ThreadPool` mutex and cond variable.
        
        // std::cout << "Enter destroy_pool()\n";
        lock_mutex(m_pool_mutex);
        
        for(int i=0 ; i<m_num_threads ; i++)
        {
            // how to clear queue? No clear() option.
            pthread_mutex_destroy(&m_thread_list[i].m_thread_mutex);
            pthread_cond_destroy(&m_thread_list[i].m_thread_cond);
        }
        
        delete[] m_thread_list;

        unlock_mutex(m_pool_mutex);

        pthread_mutex_destroy(&m_pool_mutex);
        pthread_cond_destroy(&m_pool_cond);

        // std::cout << "Exit destroy_pool()\n";
    }

    void ThreadPool::wait_cond(pthread_cond_t &cond, pthread_mutex_t &mutex)
    {
        // std::cout << "Enter wait_cond()\n";
        if (pthread_cond_wait(&cond, &mutex) < 0)
        {
            perror("[wait_cond error] : ");
            destroy_pool(); // is this necessary?
            exit(-1);
        }
        // std::cout << "Exit wait_cond()\n";
    }

    void ThreadPool::wake_cond(pthread_cond_t &cond)
    {
        // std::cout << "Enter wake_cond()\n";
        if (pthread_cond_signal(&cond) < 0)
        {
            perror("[wake_cond error] : ");
            destroy_pool(); // is this necessary?
            exit(-1);
        }
        // std::cout << "Exit wake_cond()\n";
    }

    void ThreadPool::lock_mutex(pthread_mutex_t &mutex)
    {
        // std::cout << "Enter lock_mutex()\n";
        if (pthread_mutex_lock(&mutex) < 0)
        {
            perror("[mutex_lock error] : ");
            destroy_pool(); // is this necessary?
            exit(-1);
        }
        // std::cout << "Exit lock_mutex()\n";
    }

    void ThreadPool::unlock_mutex(pthread_mutex_t &mutex)
    {
        // std::cout << "Enter unlock_mutex()\n";
        if (pthread_mutex_unlock(&mutex) < 0)
        {
            perror("[mutex_unlock error] : ");
            destroy_pool(); // is this necessary?
            exit(-1);
        }
        // std::cout << "Exit unlock_mutex()\n";
    }

} // namespace threadpool
