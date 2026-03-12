#include "response.hpp"


#define MAX_ERROR_PAGE_SIZE 1048576 // 1 MB

int getContentLenght (int fd, std::string& content, size_t readedBytes)
{
    std::cout << "\n fd = " << fd << std::endl;
    struct stat fileStat;
    if (fstat(fd, &fileStat) == -1)
        return 1;
    content = myToString(fileStat.st_size - readedBytes);
    return 0;
}

static std::string buildredirectHeader (const std::string& location, int statusCode)
{
    std::string response;

    response += "HTTP/1.1 " + myToString(statusCode) + " " + status::message(statusCode) + "\r\n";
    response += "Location: " + location + "\r\n";
    response += "Content-Length: 0\r\n";
    response += "\r\n";

    return response;
}

std::string httpResponse::buildCgiHeaders (httpRequest& req)
{
    std::string response;

    int statusCode = 200;
    std::string contentType;
    std::string contentLength;
    bool hasStatus = false;

    if (getContentLenght(req.responseFd, contentLength, req.byte_readed) != 0)
    {
        std::cerr << "Error getting content length of cgi file" << std::endl;
        return req.statusCode = 500, "";
    }

    for (size_t i = 0; i < req.cgiHeaders.size(); i++)
    {
        std::string header = req.cgiHeaders[i];
        if (!hasStatus && header.substr(0, 7) == "Status:")
        {
            std::string statusCodeStr = header.substr(8);
            std::istringstream ss(statusCodeStr);
            if (!(ss >> statusCode) || statusCode < 100 || statusCode > 599)
            {
                std::cerr << "Invalid status code in CGI output: " << statusCodeStr << std::endl;
                return req.statusCode = 500, "";
            }
            req.statusCode = statusCode;
            hasStatus = true;
        }
        if (contentType.empty() && header.substr(0, 13) == "Content-Type:")
        {
            contentType = header.substr(14);
            if (!contentType.empty() && contentType[contentType.size() - 1] == '\n')
                contentType.erase(contentType.end() - 1);
        }
        if (header.substr(0, 15) == "Content-Length:")
            continue; // ignore content length from cgi output, we will calculate it ourselves
    }

    if (contentType.empty())
    {
        std::cerr << "Content-Type header missing in CGI output" << std::endl;
        return req.statusCode = 500, "";
    }

    response += "HTTP/1.1 " + myToString(statusCode) + " " + status::message(statusCode) + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + contentLength + "\r\n";

    for (size_t i = 0; i < req.cgiHeaders.size(); i++)
    {
        std::string header = req.cgiHeaders[i];
        if (header.substr(0, 7) != "Status:" && header.substr(0, 13) != "Content-Type:" && header.substr(0, 15) != "Content-Length:")
            response += header + "\r\n";
    }

    response += "\r\n";
    
    return response;
}

std::string httpResponse::buildHeaders (httpRequest& req, int statusCode)
{
    std::string response;

    if (statusCode == 302 || statusCode == 301 || statusCode == 307 || statusCode == 308)
    {
        req.responseFd = -1; // no need to send body
        return buildredirectHeader(req.filePath, statusCode);
    }

    if (statusCode == 204)//no content y3ni mkynch body
    {
        req.responseFd = -1;
        response += "HTTP/1.1 204 No Content\r\n\r\n";
        return response;
    }

    response += "HTTP/1.1 " + myToString(statusCode) + " " + status::message(statusCode) + "\r\n";
    std::string contentType = type::get(req.filePath);
    response += "Content-Type: " + contentType + "\r\n";
    std::string contentLength;

    if (req.method == "POST")
        contentLength = "0";//content length mm7tajch l body
    else if (getContentLenght(req.responseFd, contentLength, req.byte_readed) != 0)
    {
        std::cout << "\n\n\n wech dkhlna hna \n\n\n";
        return buildError(500);
    }
    response += "Content-Length: " + contentLength + "\r\n";
    if (!req.cookiesHeaders.empty())
    {
        for (size_t i = 0; i < req.cookiesHeaders.size(); i++)
            response += req.cookiesHeaders[i] + "\r\n";
    }

    response += "\r\n";

    return response;
}

std::string httpResponse::build (httpRequest& req)
{
    std::string response;
    if (req.responseFd == -1)
        return "";
    char buffer[4096] ="";
    ssize_t bytesRead = read(req.responseFd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        response = std::string(buffer, bytesRead);
    }
    else if (bytesRead == 0)
    {
        close(req.responseFd); // file completely read
        req.responseFd = -1;
    }
    else
        return req.statusCode = 500, ""; // error reading file, mazal ma3reftch kifach nhandleha f had l7ala
    std::cout << "\n\n\nresponse from build: " << response.size() <<"\n\n\n"<< std::endl;
    return response;
}


std::string httpResponse::buildError (int statusCode)
{
    std::string response;

    response += "HTTP/1.1 " + myToString(statusCode) + " " + status::message(statusCode) + "\r\n";

    std::string file;
    std::map<int, std::string> errorPages = srv.getErrorPages();
    std::map<int, std::string>::iterator it = errorPages.find(statusCode);
    if (it != errorPages.end()) // ila jatni error page f config file cansiftha
    {
        file = "./" + srv.getRoot() + it->second;
        std::cout << "\n\n\n error page file: " << file << "\n\n\n";
        std::ifstream errorFile(file.c_str(), std::ios::binary);
        struct stat fileStat;
        if (stat(file.c_str(), &fileStat) == 0 && fileStat.st_size <= MAX_ERROR_PAGE_SIZE)
        {
            response += "Content-Length: " + myToString(fileStat.st_size) + "\r\n";
            response += "Content-Type: text/html\r\n";
            response += "\r\n";
            std::stringstream buffer;
            buffer << errorFile.rdbuf();
            response += buffer.str();
            return response;
        }
        else
        {
            // If the file is too large or cannot be accessed, fall back to simple message
            response += "Content-Length: 3\r\n";
            response += "Content-Type: text/plain\r\n";
            response += "\r\n";
            response += myToString(statusCode) + " ";
            response += " \r\n";
            return response;
        }
    }
    else // ila majatnish f config file kansift gha status cde f body
    {
        response += "Content-Length: 3\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "\r\n";
        response += myToString(statusCode) + " ";
        response += " \r\n";
    }

    return response;
}

// std::string httpResponse::buildError(int statusCode)//bghit nteste bhad minimal response t9der trje3 dialk mn b3d
// {
//     // Body: keep it simple (no trailing newline unless you want it)
//     std::string body = myToString(statusCode);

//     std::string response;
//     response += "HTTP/1.1 " + myToString(statusCode) + " " + status::message(statusCode) + "\r\n";
//     response += "Content-Type: text/plain\r\n";
//     response += "Content-Length: " + myToString(body.size()) + "\r\n";
//     response += "\r\n";
//     response += body;

//     return response;
// }

std::string httpResponse::buildAutoindex (httpRequest& req)
{
    std::string response;

    std::string dirPath = "./" + srv.getRoot() + req.path;
    DIR* dir = opendir(dirPath.c_str());
    if (!dir)
    {
        if (errno == ENOENT || errno == ENOTDIR) // directory does not exist or is not a directory
            return buildError(404);
        if (errno == EACCES) // permission denied
            return buildError(403);
    }

    std::vector<std::string> entries;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name != "." && name != "..")
            entries.push_back(name);
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());

    std::string body = "<html><head><title>Directory Listing for path: " + req.path + "</title></head><body><h2>Directory Listing for path: " + req.path + "</h2><ul>";
    for (size_t i = 0; i < entries.size(); i++)
        body += "<li><a href=\"" + entries[i] + "\">" + entries[i] + "</a></li>";
    body += "</ul></body></html>";

    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + myToString(body.size()) + "\r\n";
    response += "\r\n";
    response += body;

    return response;
}
