#include "client_class.hpp"
#include "../parsingRequest/parsing_request.hpp"
#include "../config/config.hpp"
#define MAX_EVENTS 1024



int make_it_non_block(int fd)
{
    int flags = fcntl(fd,F_GETFL, 0);
    if(flags == -1)
    {
        return(-1);
    }
    return(fcntl(fd, F_SETFL, flags | O_NONBLOCK));
}


int is_listen_fd(int fd, std::vector<int>& serversfd)
{
    return (std::find(serversfd.begin(), serversfd.end(), fd) != serversfd.end());
}


