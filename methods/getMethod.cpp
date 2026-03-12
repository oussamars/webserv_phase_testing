#include "../parsingRequest/parsing_request.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <fcntl.h>

// Simple helper: trim leading '/'
std::string trimLeadingSlash(const std::string& p)
{
    size_t i = 0;

    if (p.empty())
        return p;
    while (i < p.size() && p[i] == '/')
        ++i;
    return p.substr(i);
}

// Prevent naive path traversal (reject .. segments)
bool hasPathTraversal(const std::string& p)
{
    return p.find("../") != std::string::npos || p == ".." || p.find("/..") != std::string::npos;
}

// Pick best matching location by longest prefix match
const LocationConfig* findBestLocation(const ServerConfig& server, const std::string& path)
{
    const LocationConfig* target = NULL;
    size_t len = 0;

    std::string trimedPath = pathWithOneSlash(path);
    if (trimedPath.empty())
        return NULL;
    
    // Normalize: ensure path ends with / for directory matching
    if (trimedPath[trimedPath.size() - 1] != '/')
        trimedPath += '/';
    
    for (size_t i = 0; i < server.getLocations().size(); ++i)
    {
        const LocationConfig& loc = server.getLocations()[i];
        std::string prefix = pathWithOneSlash(loc.getPath());
        if (prefix.empty())
            continue;
        
        // Normalize: ensure prefix ends with /
        if (prefix[prefix.size() - 1] != '/')
            prefix += '/';
        
        if (trimedPath.compare(0, prefix.size(), prefix) == 0)
        {
            if (prefix.size() > len)
            {
                target = &loc;
                len = prefix.size();
            }
        }
    }
    return target;
}

int httpRequest::handleGetRequest(const ServerConfig& srv)
{
    // if (!parsingRequestComplete)
    //     return -1;

    const std::vector<ServerConfig>& servers = globalConfig.getServers();
    if (servers.empty()) {
        return this->statusCode = 500, 1; // 500 => Internal Server Error
    }
    // const ServerConfig& srv = servers[0]; // ghid normalement ikhssa agis st3melgh server limod tochka request, mais radskregh lewel haliyan.

    // kan9leb 3la location li katnassab had request
    const LocationConfig* location = findBestLocation(srv, this->path);

    // checking if the method is allowed in this location
    if (location)
    {
        const std::vector<std::string>& allowedMethods = location->getAllowedMethods();
        if (!allowedMethods.empty())
        {
            if (std::find(allowedMethods.begin(), allowedMethods.end(), this->method) == allowedMethods.end())
            {
                return this->statusCode = 405, 1; // method not allowed
            }
        }
    }

    if (location && !location->getRedirect().empty())
    {
        std::string redirectURL = location->getRedirect();
        this->filePath = redirectURL;
        if (redirectURL.substr(0, 4) == "http")
        {
            return this->statusCode = 301, 0; // 301 → redirect to a final, permanent URL (often a full external URL like https://youtube.com)
        }
        return this->statusCode = 302, 0; // 302 → redirect to a temporary URL (often a relative URL on the same server)
    }

    // Resolve filesystem path
    std::string trimedPath = pathWithOneSlash(this->path);
    if (hasPathTraversal(trimedPath))
        return this->statusCode = 403, 1; // path traversal (403 => forbidden)

    std::string root;
    // if (!location)
    root = srv.getRoot();
    // else
    //     root = location->getRoot();
    root += "/";
    std::string fullPath = root + trimedPath;
    fullPath = "./" + fullPath; // assuming server root is relative to current directory

    // if it is a directory, try to append index file
    // struct stat st;
    // bool isDir = (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
    // if (isDir) {
    //     if (location && !location->getIndex().empty())
    //         fullPath += location->getIndex();
    //     else
    //         fullPath += "/index.html";
    // }


    // std::cout << "Full path resolved: " << fullPath << std::endl;

    std::ifstream file(fullPath.c_str(), std::ios::binary);
    if (!file.good())
        return this->statusCode = 404, 1; // not found

    this->responseFd = open(fullPath.c_str(), O_RDONLY);
    if (this->responseFd == -1)
        return this->statusCode = 500, 1;
    
    this->filePath = fullPath;
    this->statusCode = 200;
    return 0;
}