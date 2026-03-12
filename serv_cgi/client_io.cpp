#include "client_class.hpp"
#include "../parsingRequest/parsing_request.hpp"
#include "../config/config.hpp"
#define MAX_EVENTS 1024



// int accepting_client(int listenfd)
// {
//     struct sockaddr_in  client;
//     socklen_t  len = sizeof(client);
//     int client_fd = accept(listenfd, (struct sockaddr *)&client, &len);
//     if (client_fd < 0)
//     {
//         if(errno == EAGAIN || errno == EWOULDBLOCK)
//         {
//             return(-1);
//             // break;
//         }
//         if(errno == EINTR)
//         {
//             return(-2);
//             // continue;DELETE
//         }
//         perror("accept");
//         return(-1);
//         // break;
//     }
//     if(make_it_non_block(client_fd) < 0)
//     {
//         perror("fcntl(client_fd)");
//         close(client_fd);
//     }
//     return(client_fd);
// }




std::string extract_Cookies(const std::map<std::string, std::string>& headers)
{
    std::map<std::string, std::string>::const_iterator it = headers.find("cookie");
    if (it != headers.end())
    {
        std::string cookies = it->second;
        size_t pos = cookies.find("session_id=");
        if (pos != std::string::npos)
        {
            return cookies.substr(pos + 11);
        }
    }
    return "";
}

std::string generate_cookies()
{
    std::stringstream ss;

    ss << std::time(NULL) << rand();

    return ss.str();
}


void accept_clients(int fd, int epfd, std::map<int, client_class>& client_list, ServerConfig &server)
{

    int clientfd = accept(fd, NULL, NULL);
    if (clientfd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        return ; // no more clients
        perror("accept");
    }
    make_it_non_block(clientfd);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = clientfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev) == -1)
    {
        perror("epoll_ctl client");
        close(clientfd);
        return ;
    }
    std::cout<<"\n\n\n accepte client withe socket fd= "<< clientfd <<"\n\n\n";
    client_list.insert(std::make_pair(clientfd, client_class(clientfd, server)));

}



void close_client(int fd, int epfd, std::map<int, client_class>& client_list)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    client_list.erase(fd);
}


void read_request(int fd, int epfd, std::map<int, client_class>& client_list)
{
    char buf[4096];

    ssize_t r = recv(fd, buf, sizeof(buf), 0);
    std::map<int, client_class>::iterator it = client_list.find(fd);
    if (r > 0)
    {
        if (it == client_list.end())
            return;

        it->second.append(buf, r);
        int rc = parseHttpRequest(it->second.getbuffer(), it->second.getrequest());

        /////////////////////////////////////////////
        if (rc == 1)
        {
            //we need data chouf chno ghdir hna
            return;
        }
        if (rc >= 400)
        {
            std::string errResp = it->second.getresponse().buildError(rc);
            if (!errResp.empty())
                write(fd, errResp.c_str(), errResp.size());
            it->second.headerSent = true;
            it->second.responseDone = true;
            close_client(fd, epfd, client_list);
            return;
        }
        /////////////////////////////////////////
        if (it->second.getrequest().parsingRequestComplete)
        {
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
            ev.data.fd = fd;

            if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
                perror("epoll_ctl MOD");
        }
    }
    else if (r == 0)
    {
        // client closed connection
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        client_list.erase(fd);
    }
    else
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        perror("recv");
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        client_list.erase(fd);
    }
}



void send_response(int epfd, int fd, std::map<int, client_class>& client_list, std::vector<std::string> &users, std::map<std::string,std::string> &sessions)
{
    std::map<int, client_class>::iterator it = client_list.find(fd);
    if (it != client_list.end() &&  !it->second.responseDone)
    {
        std::string resp;
        std::string resp1;
        client_class &cl = it->second;
        if(cl.responseDone)
        {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            if (std::remove(cl.file_cgi.c_str()) == -1)
            {
                std::cerr << "nothing to lose \n";
            }
            client_list.erase(fd);
            std::cout << "No response to send for fd=" << fd << "\n";
            return ;
        }
    if (cl.getserver().getRoot() == "/website")
    {

        std::string cookies = extract_Cookies(cl.getrequest().headers);
        std::cout<<"\n\n\n cookies extracted: " << cookies << "\n\n\n";
        if(cl.getrequest().method == "GET" && cl.getrequest().path == "/index.html")
        {
            
            cl.is_logged_in = false;
            cl.cookies = "deleted";
            cl.getrequest().cookiesHeaders.clear();
            cl.getrequest().cookiesHeaders.push_back("Set-Cookie: session_id=deleted");
            cl.getrequest().cookiesHeaders.push_back("Cache-Control: no-store, no-cache, must-revalidate");

            std::map<std::string, std::string>::iterator it = sessions.find(cookies);
            if(it != sessions.end())
            {
                sessions.erase(it);
            }
            cl.username = "";

        }else if(cl.getrequest().method == "GET" && cl.getrequest().path == "/login.html")
        {
            std::map<std::string, std::string>::iterator it = sessions.find(cookies);
            if(it != sessions.end())
            {                
                cl.is_logged_in = true;
                cl.username = it->first;
                cl.cookies = it->second;
                cl.getrequest().cookiesHeaders.clear();
                cl.getrequest().cookiesHeaders.push_back("Set-Cookie: session_id=" + cl.cookies);
                cl.getrequest().cookiesHeaders.push_back("Cache-Control: no-store, no-cache, must-revalidate");
                cl.getrequest().method = "GET";
                cl.getrequest().path = "/index.html";
            } else
            {  
                cl.getrequest().method = "GET";
                cl.getrequest().path = "/login.html";
            }
        } else if(cl.getrequest().method == "POST" && cl.getrequest().path == "/dashboard.html")
        {
            std::cout<<"\n\n\n" << cl.getrequest().body << "\n\n\n";
            std::vector<std::string>::iterator it = std::find(users.begin(), users.end(), cl.getrequest().body);
            if(it == users.end() && !cl.is_logged_in)
            {
                cl.getrequest().method = "GET";
                cl.getrequest().path = "/signin.html";
                return;
            }
            if(sessions.find(cookies) == sessions.end())
            {
                cl.is_logged_in = true;
                cl.username = cl.getrequest().body;
                cl.cookies = generate_cookies();
                sessions[cl.cookies] = cl.username;
                std::stringstream size_body;
                size_body << resp1.size();
                cl.getrequest().cookiesHeaders.clear();
                cl.getrequest().cookiesHeaders.push_back("Set-Cookie: session_id=" + cl.cookies);
                cl.getrequest().cookiesHeaders.push_back("Cache-Control: no-store, no-cache, must-revalidate");
                cl.getrequest().method="GET";
                cl.getrequest().path="/dashboard.html";
            } else
            {
                cl.is_logged_in = true;
                cl.getrequest().cookiesHeaders.clear();
                cl.getrequest().cookiesHeaders.push_back("Set-Cookie: session_id=" + cl.cookies);
                cl.getrequest().cookiesHeaders.push_back("Cache-Control: no-store, no-cache, must-revalidate");
                cl.getrequest().method="GET";
                cl.getrequest().path="/dashboard.html";
            }

        } else if(cl.getrequest().method == "POST" && cl.getrequest().path == "/added.html" )
        {
            std::vector<std::string>::iterator it = std::find(users.begin(), users.end(), cl.getrequest().body);
            if(it != users.end())
            {
                cl.getrequest().method = "GET";
                cl.getrequest().path = "/register.html";
            } else
            {
                users.push_back(cl.getrequest().body);
                cl.getrequest().method = "GET";
                cl.getrequest().path = "/added.html";
            }
        }
    }
        if (!cl.headerSent)
        {
            const LocationConfig* location = findBestLocation(cl.getserver(), cl.getrequest().path);

            if (!location && (cl.getrequest().method == "GET" || cl.getrequest().method == "DELETE"))
            {
                std::cout << "\nsection -> 1 : request path = " << cl.getrequest().path << "\n";
                cl.getrequest().appendIndexIfDirectory(
                    cl.getserver().getRoot(),
                    cl.getserver().getIndex());
            }
            else if (location && location->getIndex().empty() && location->isAutoindex() && cl.getrequest().method == "GET")
            {
                std::cout << "\nsection -> 2\n";
                std::string autoindexResp = cl.getresponse().buildAutoindex(cl.getrequest());
                if (!autoindexResp.empty())
                    write(fd, autoindexResp.c_str(), autoindexResp.size());
                cl.headerSent = true;
                cl.responseDone = true;
                return;
            }
            else if (location && !location->getIndex().empty() && cl.getrequest().method == "GET")
            {
                std::cout << "\nsection -> 3\n";
                cl.getrequest().appendIndexIfDirectory(
                    cl.getserver().getRoot(),
                    location->getIndex());
            }
            else if (location && location->getIndex().empty() && !location->isAutoindex())
            {
                std::cout << "\nsection -> 4\n";
                std::string errResp = cl.getresponse().buildError(403);
                if (!errResp.empty())
                    write(fd, errResp.c_str(), errResp.size());
                cl.headerSent = true;
                cl.responseDone = true;
                return;
            }
            resp = wiche_met(cl);

            if (!cl.is_cgi)
            {
                if (cl.getrequest().statusCode >= 400)
                    resp = cl.getresponse().buildError(cl.getrequest().statusCode);
                else
                    resp = cl.getresponse().buildHeaders(cl.getrequest(), cl.getrequest().statusCode);

                write(fd, resp.c_str(), resp.size());
                cl.headerSent = true;
            }
        }

        if (cl.headerSent && !cl.responseDone)
        {
            resp1 = cl.getresponse().build(cl.getrequest());

            if (!resp1.empty())
                write(fd, resp1.c_str(), resp1.size());
            else
                cl.responseDone = true;
        }
        if (cl.responseDone)
        {
            close(fd);
            client_list.erase(fd);
        }
    }
    else
    {
        if (it == client_list.end())
        {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
        }
        if (it->second.responseDone)
        {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            client_list.erase(fd);
            close(fd);
        }
    }
}