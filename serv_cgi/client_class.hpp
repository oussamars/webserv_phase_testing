#pragma once
#include <cerrno>
#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include <fcntl.h>
#include <map>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>
#include <poll.h>
#include <string>
#include <sstream>
#include "../parsingRequest/parsing_request.hpp"
#include "../Response/response.hpp"
#include <sys/wait.h>
#include <cstdio>
#include <ctime>

// ServerConfig is already defined via config.hpp included from parsing_request.hpp
// httpRequest is already defined in parsing_request.hpp
// httpResponse is already defined in response.hpp

class client_class
{
    private:
        int socket_fd;
        httpRequest request_;
        httpResponse response_;
        std::string buffer;
        std::string re;
        ServerConfig server_;
    public:
        int isoupen;
        bool responseDone;
        bool headerSent;
        std::string file_hard;
        static unsigned long hard;
        std::string file_cgi;
        int cgi_pid;
        bool is_cgi;
        bool is_executed;
        bool is_running;
        int ret;
        time_t time_out;
        std::string username;
        std::string cookies;
        bool is_logged_in;
        client_class(int fd, const ServerConfig &server)
            : socket_fd(fd),
            request_(),
            response_(server),
            buffer(""),
            re(""),
            server_(server),
            isoupen(1),
            responseDone(false),
            headerSent(false),
            file_cgi(""),
            is_cgi(false),
            is_executed(false),
            is_running(false),
            ret(0),
            time_out(std::time(NULL)),
            username(""),
            cookies(""),
            is_logged_in(false)
        {
        }

        httpRequest& request() { return request_; }
        const httpRequest& request() const { return request_; }


        void manage () {
            // check the end of the rquest
            // call the request (buffer);
            // call the response (request, client);
        }
        void setbuffer(const std::string& s1, const std::string& s2)
        {
            buffer = s1 + s2;
        }
        void setre(const std::string& s1, const std::string& s2)
        {
            re = s1 + s2;
        }
        void append(const char* buff, ssize_t r)
        {
            buffer.append(buff, buff+r);
        }
        std::string& getbuffer()
        {
            return(this->buffer);
        }
        std::string& getre()
        {
            return(this->re);
        }
        httpRequest& getrequest()
        {
            return(this->request_);
        }
        httpResponse& getresponse()
        {
            return(this->response_);
        }

        // abifkirn
        void setsocket_fd(int fd)
        {
            this->socket_fd = fd;
        }
        int getfd()
        {
            return(this->socket_fd);
        }
        const ServerConfig& getserver() const
        {
            return(this->server_);
        }
        ~client_class(){}
};


std::string wiche_met(client_class &cl);
int handelServers(std::vector<std::string> &users, std::map<std::string,std::string> &sessions);
void send_response(int epfd, int fd, std::map<int, client_class>& client_list, std::vector<std::string> &users, std::map<std::string,std::string> &sessions);
void close_client(int fd, int epfd, std::map<int, client_class>& client_list);
void read_request(int fd, int epfd, std::map<int, client_class>& client_list);
void accept_clients(int fd, int epfd, std::map<int, client_class>& client_list, ServerConfig &server);
int is_listen_fd(int fd, std::vector<int>& serversfd);
std::string wiche_met(client_class &cl);
struct pollfd create_client(int client_fd, std::map<int, client_class>& read_cl, std::map<int, client_class>& write_cl, const ServerConfig &server);
int accepting_client(int listenfd);
int make_it_non_block(int fd);
bool is_cgi(const ServerConfig &server, std::string& path);
int handle_cgi(client_class& cl);
int ft_getline(std::string &headerx, client_class& cl, int fd);
bool checkHeader(const std::string &headerx, int a);
void give_headers(client_class &cl);
bool isallowed(const client_class& cl, const std::string& method);










