#ifndef PARSING_REQUEST_HPP
#define PARSING_REQUEST_HPP

#include <climits>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../config/config.hpp"

std::string trimLeadingSlash(const std::string& p);
bool hasPathTraversal(const std::string& p);
const LocationConfig* findBestLocation(const ServerConfig& server, const std::string& path);


#define MAX_BODY_SIZE 2000000


class client_class;


class httpRequest
{
public:
    std::string method;
    std::string path;
    std::string httpVersion;
    std::string queryString;
    std::string cookieHeader;

    std::map<std::string, std::string> headers;
    std::string body;

    size_t headersBytes;
    size_t bodyBytes; 
    size_t totalBytesConsumed;
    std::vector<std::string> redirectCockis;

    bool parsingRequestComplete;

    int statusCode;    
    int responseFd;
    std::string filePath;
    /////////////////////////////
    std::vector<std::string> cookiesHeaders;
    ///////////////////////////


    //zakaria t9der tb9a t3yet 3la req.reset() mn mor kola request bech itresetaw l attributes
    httpRequest()
        : headersBytes(0),
          bodyBytes(0),
          totalBytesConsumed(0),
          parsingRequestComplete(false),
          statusCode(0),
          responseFd(-1),
          byte_readed(0)
    {}

    void parseRequestLine(const std::string& requestLine);
    void parseHeaders(const std::string& headersBlock);
    void parseBody(const std::string& bodyData);

    int validateRequest();

    std::vector<std::string> cgiHeaders;

    int handlePostRequest(const ServerConfig& srv);
    int handleGetRequest(const ServerConfig& srv);
    int handleDeleteRequest(const ServerConfig& srv);
    void appendIndexIfDirectory(const std::string& rootPath, const std::string& indexFile);

    size_t byte_readed;

   

};

int parseHttpRequest(const std::string& buf, httpRequest& request);

std::string trimLeadingSpaces(const std::string& str);


#endif
