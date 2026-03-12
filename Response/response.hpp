#pragma once

#include "../parsingRequest/parsing_request.hpp"
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../config/config.hpp"
#include "response.hpp"
#include "status.hpp"
#include "contentType.hpp"

class httpResponse
{
    private:
        ServerConfig srv;
    public:
        httpResponse(const ServerConfig& config) : srv(config) {};
        std::string buildHeaders (httpRequest& req, int statusCode);
        std::string build (httpRequest& req);
        std::string buildError (int statusCode);
        std::string buildCgiHeaders (httpRequest& req);
        std::string buildAutoindex (httpRequest& req);
};
