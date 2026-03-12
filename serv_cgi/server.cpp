#include "client_class.hpp"
#include "../parsingRequest/parsing_request.hpp"
#include "../config/config.hpp"
#define MAX_EVENTS 1024


int handelServers(std::vector<std::string> &users, std::map<std::string,std::string> &sessions)
{
    std::vector<int> serversfd;
    std::map<int, client_class> client_list;
    std::vector<ServerConfig> serverslist = globalConfig.getServers();
    std::map<int, ServerConfig> listen_fd_to_server;

    for(size_t j = 0; j < serverslist.size(); j++)
    {
        int listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if(listenfd == -1)
        {
            std::perror("server socket");
            return(1);
        }
        // std::cout << "Created server socket fd: " << listenfd << " for server " << j << std::endl;
        serversfd.push_back(listenfd);
        listen_fd_to_server[listenfd] = serverslist[j];
    }
    int option = 1;
    for(size_t j = 0; j < serversfd.size(); j++)
    {
        if(setsockopt(serversfd[j], SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
        {
            perror("setsockopt");
            close(serversfd[j]);
            serversfd.erase(serversfd.begin() + j);
            listen_fd_to_server.erase(serversfd[j]);
            serverslist.erase(serverslist.begin() + j);
            j--;
            return 1;
        }
    }
    struct sockaddr_in addr[serversfd.size()];
    std::memset(&addr, 0, sizeof(addr));
    
    for(size_t i = 0; i < serversfd.size(); i++)
    {
        addr[i].sin_family = AF_INET;
        addr[i].sin_addr.s_addr = INADDR_ANY;
        addr[i].sin_port = htons(serverslist[i].getPort());
    }

    for(size_t j = 0; j < serversfd.size(); j++)
    {
        if(bind(serversfd[j], (struct sockaddr *)&addr[j], sizeof(addr[j])) < 0)
        {
            perror("bind");
            close(serversfd[j]);
            serversfd.erase(serversfd.begin() + j);
            listen_fd_to_server.erase(serversfd[j]);
            serverslist.erase(serverslist.begin() + j);
            j--;
            return 1;
        }
    }
    for(size_t j = 0; j < serversfd.size(); j++)
    {
        if (listen(serversfd[j], 128) < 0)
        {
            std::cout<<"listen failed on server fd "<< serversfd[j]<<" with error: " << strerror(errno) << std::endl;
            perror("listen");
            close(serversfd[j]);
            serversfd.erase(serversfd.begin() + j);
            listen_fd_to_server.erase(serversfd[j]);
            serverslist.erase(serverslist.begin() + j);
            return 1;
        }
        // std::cout<<"listen success on server fd "<< serversfd[j]<< std::endl;
    }
    for(size_t j = 0; j < serversfd.size(); j++)
    {
        if (make_it_non_block(serversfd[j]) < 0)
        {
            perror("fcntl");
            close(serversfd[j]);
            serversfd.erase(serversfd.begin() + j);
            listen_fd_to_server.erase(serversfd[j]);
            serverslist.erase(serverslist.begin() + j);
            j--;
            return 1;
        }
    }
    int epfd = epoll_create1(0);
    struct epoll_event ev;
    for(size_t i = 0; i < serversfd.size(); i++)
    {
        ev.events = EPOLLIN;
        ev.data.fd = serversfd[i];
        epoll_ctl(epfd, EPOLL_CTL_ADD, serversfd[i], &ev);
    }
    struct epoll_event events[MAX_EVENTS];

    while (true)
    {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        if (nfds < 0)
        {
            if (errno == EINTR)
                continue ;
            perror("epoll_wait");
            break;
        }

        //
        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;
            uint32_t evs = events[i].events;

           if (is_listen_fd(fd,serversfd))
            {
                accept_clients(fd, epfd, client_list, listen_fd_to_server[fd]);
            }
            else if (evs & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                close_client(fd, epfd, client_list);
            }
            else if (evs & EPOLLIN)
            {
                read_request(fd, epfd, client_list);
            }
            else if (evs & EPOLLOUT)
            {
                send_response(epfd ,fd, client_list, users, sessions);
            }
        }
        std::map<int, client_class>::iterator it1 = client_list.begin();
        while (it1 != client_list.end())
        {
            time_t time_now(std::time(NULL));
            
            if((time_now - (it1->second.time_out)) > 6 )
            {
                std::map<int, client_class>::iterator it2 = it1;
                it1++;
                if(it2->second.is_running)
                {
                    kill(it2->second.cgi_pid, SIGKILL);
                    it2->second.is_running = false;
                    it2->second.is_executed = true;

                }
                if(it2->second.is_cgi)
                    it2->second.getrequest().statusCode = 504;
                else
                    it2->second.getrequest().statusCode = 408;
                std::string response = it2->second.getresponse().buildError(it2->second.getrequest().statusCode);
                write(it2->second.getfd(), response.c_str(), response.size());
                it2->second.headerSent = true;
                std::string resp = it2->second.getresponse().build(it2->second.getrequest());
                write(it2->second.getfd(), resp.c_str(), resp.size());
                it2->second.responseDone = true;
                epoll_ctl(epfd, EPOLL_CTL_DEL, it2->second.getfd(), NULL);
                close(it2->second.getfd());
                client_list.erase(it2);
                continue;
            }
            
            if (it1->second.is_running)
            {
                int status;
                pid_t ret = waitpid(it1->second.cgi_pid, &status, WNOHANG);
                
                if (ret == 0)
                {
                    ++it1;
                    continue;
                }
                
                if (ret > 0)
                {
                    it1->second.is_executed = true;
                    it1->second.is_running = false;

                    if (WIFEXITED(status))
                    {
                        int exit_status = WEXITSTATUS(status);
                        it1->second.getrequest().statusCode =
                            (exit_status == 0) ? 200 : 500;
                    }
                    else
                        it1->second.getrequest().statusCode = 500;

                    if (it1->second.getrequest().responseFd != -1)
                    {

                        close(it1->second.getrequest().responseFd);

                        if ((it1->second.getrequest().responseFd =
                                open(it1->second.file_hard.c_str(), O_RDONLY)) < 0)
                        {
                            perror("open");
                            return (1);
                        }

                        std::string headerx;
                        ssize_t r = 0;
                        int a = 0;

                        while ((r = ft_getline(headerx,
                                it1->second,
                                it1->second.getrequest().responseFd)) > 0)
                        {
                            if (headerx == "\n" ||
                                headerx == "\r\n" ||
                                headerx.empty())
                                break;

                            if (!checkHeader(headerx, a))
                            {
                                it1->second.getrequest().statusCode = 500;
                                break;
                            }

                            if (it1->second.getrequest().cgiHeaders.size() >= 10)
                            {
                                it1->second.getrequest().statusCode = 500;
                                break;
                            }

                            it1->second.getrequest().cgiHeaders.push_back(headerx);
                            a++;
                        }
                        if (it1->second.ret == 0)
                        {
                            give_headers(it1->second);
                        }
                    }
                }
            }
            it1++;
        }
    }

    return(0);
}