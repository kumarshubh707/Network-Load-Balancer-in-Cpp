#include <iostream>
#include "threadpool.h"
#include "server.h"


int main(int argc, char** argv)
{
    if(argc!=3)
    {
        std::cout << "Invalid use : correct usge : ./load_balancer port_number num_threads\n";
        exit(-1);
    }

    server::ServerBase server;
    server.startServer(argv[1], atoi(argv[2]));
    
    return 0;
}
