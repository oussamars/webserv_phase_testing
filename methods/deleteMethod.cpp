#include "../parsingRequest/parsing_request.hpp"


int httpRequest::handleDeleteRequest(const ServerConfig& srv)
{
    // if (!parsingRequestComplete)
    //     return -1;

    const std::vector<ServerConfig>& servers = globalConfig.getServers();
    if (servers.empty()) {
        std::cerr << "No servers configured in globalConfig." << std::endl;
        return this->statusCode = 500, 1;
    }
    // const ServerConfig& srv = servers[0];

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
            return this->statusCode = 308, 0; // 308 → does the same as 301 + keeps the method unchanged, cause if 301/302 is used that might change DELETE to GET
        }
        return this->statusCode = 307, 0; // 307 → does the same as 302 + keeps the method unchanged (the problem of method changing happens only in DELETE/POST)
    }

    std::string trimedPath = pathWithOneSlash(this->path);
    if (hasPathTraversal(trimedPath))
        return this->statusCode = 403, 1; // path traversal (403 => forbidden)

    std::string root;
    // if (!location)
    root = srv.getRoot();
    // else
    //     root = location->getRoot();
    root += '/';
    std::string fullPath = "." + root + trimedPath; // ila mazdnach "." ghadi i9leb f root system

    // Attempt to delete the file
    // std::cout << "Attempting to delete file: " << fullPath << std::endl;
    if (std::remove(fullPath.c_str()) != 0)
        return this->statusCode = 404, 1; // file not found or could not be deleted
    this->statusCode = 204; // No Content, f lhala dyal delete 204 hiya success
    return 0;
}