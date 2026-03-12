#include "parsing_request.hpp"
#include <algorithm>
#include <cctype>

std::string trimSpaces(const std::string& s)
{
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n'))
        b++;
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n'))
        e--;
    return s.substr(b, e - b);
}


int parseHttpRequest(const std::string& buffer, httpRequest& request)
{
    request.parsingRequestComplete = false;
    
    // 1 parse l header
    size_t headersEndPosition = buffer.find("\r\n\r\n");
    if (headersEndPosition == std::string::npos)
        return 1; // i need more data

    std::string headersBlock = buffer.substr(0, headersEndPosition);

    size_t requestLineEnd = headersBlock.find("\r\n");

    std::string requestLine;
    std::string headerLines;

    if (requestLineEnd == std::string::npos)
    {
        // valid case: request line only, no headers
        requestLine = headersBlock;
        headerLines.clear();
    }
    else
    {
        requestLine = headersBlock.substr(0, requestLineEnd);
        headerLines = headersBlock.substr(requestLineEnd + 2);
    }

    request.parseRequestLine(requestLine);
    if (request.statusCode >= 400)
    {
        request.parsingRequestComplete = true;
        return request.statusCode;
    }

    request.parseHeaders(headerLines);

    // 2 validi request structur
    //detecte malformed header bech n7bsso
    if (request.statusCode >= 400)
    {
        request.parsingRequestComplete = true;
        return request.statusCode;
    }

    int validationStatus = request.validateRequest();
    if (validationStatus != 0)
    {
        request.statusCode = validationStatus;
        request.parsingRequestComplete = true;
        return validationStatus;
    }

    // 3 parse body
    std::string bodyData = buffer.substr(headersEndPosition + 4);
    request.parseBody(bodyData);

    if (request.statusCode >= 400)
    {
        request.parsingRequestComplete = true;
        return request.statusCode;//invalid content length directly return 400
    }

    if (!request.parsingRequestComplete)
        return 1; // need more data for body

    // 4 had lvariables sefthom l zakaria bech i3ref fen badya request jaya
    request.headersBytes = headersEndPosition + 4;
    request.bodyBytes = request.body.size();
    request.totalBytesConsumed = request.headersBytes + request.bodyBytes;
    return 0;
}


void httpRequest::parseRequestLine(const std::string& line)
{
    size_t firstSpace = line.find(' ');
    size_t secondSpace;
    if (firstSpace == std::string::npos)
        secondSpace = std::string::npos;
    else
        secondSpace = line.find(' ', firstSpace + 1);

    if (firstSpace == std::string::npos || secondSpace == std::string::npos)
    {
        statusCode = 400;
        parsingRequestComplete = true;
        return; // request line mamgadach
    }

    method = line.substr(0, firstSpace);
    std::string target = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::cout << "Target before decoding: " << target << std::endl;
    httpVersion = line.substr(secondSpace + 1);

    //hna zedt query string li moumkinn t7taj a ZAKARIA f CGI
    size_t q = target.find('?');
    if (q == std::string::npos)
    {
        path = target;
        queryString.clear();
    }
    else
    {
        path = target.substr(0, q);
        queryString = target.substr(q + 1);
    }
}

void httpRequest::appendIndexIfDirectory(const std::string& rootPath, const std::string& indexFile)
{
    std::string fullPath = "." + rootPath + this->path; 
    
    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
    {
        if (this->path[this->path.size() - 1] != '/')
            this->path += "/";
        this->path += indexFile;
    }
}

void httpRequest::parseHeaders(const std::string& headersBlock)
{
    std::istringstream headerStream(headersBlock);
    std::string line;

    while (std::getline(headerStream, line))
    {
        //remove  '\r' produced by getline
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        //mkhss tkon tachi empty line fl headerblock
        if (line.empty())
            continue;
        

        size_t colonPosition = line.find(':');
        if (colonPosition == std::string::npos)
        {
            statusCode = 400;
            parsingRequestComplete = true;
            return; // header line mamgadach(malformed)
        }

        std::string headerName  = trimSpaces(line.substr(0, colonPosition));
        std::string headerValue = trimSpaces(line.substr(colonPosition + 1));

        if (headerName.empty())
        {
            statusCode = 400;
            parsingRequestComplete = true;
            return;
        }

        // rj3 header keys lowercase
        for (size_t i = 0; i < headerName.size(); ++i)
            headerName[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(headerName[i])));// so hykoun sahl n9lbo 3lihom

        headers[headerName] = headerValue;
        if (headerName == "cookie")
            cookieHeader = headerValue;

    }
}


void httpRequest::parseBody(const std::string& receivedBodyData)
{
    if (headers.find("content-length") == headers.end())
    {
        bodyBytes = 0;
        parsingRequestComplete = true;
        return;
    }

    const std::string& contentLengthValue = headers["content-length"];

       //converti string l number
    long long expectedBodyLengthLL = 0;
    {
        std::istringstream iss(contentLengthValue);
        char extra = '\0';
        if (!(iss >> expectedBodyLengthLL) || (iss >> extra) || expectedBodyLengthLL < 0)
        {
            statusCode = 400;
            parsingRequestComplete = true;
            return;
        }
    }

    //fcgi moumkin itssaft chi body l post kbir dkchi 3lach knlimittiwh bech maycrashich oula isslowe lina l event loop
    if (expectedBodyLengthLL > static_cast<long long>(MAX_BODY_SIZE))
    {
        statusCode = 413;
        parsingRequestComplete = true;
        return;
    }

    const size_t expectedBodyLength = static_cast<size_t>(expectedBodyLengthLL);

    // wech receivina lbody kaml
    if (body.size() >= expectedBodyLength)
    {
        parsingRequestComplete = true;
        return;
    }

    //hna hnbdaw nappendiw bytes jdad li receivina
    const size_t alreadyHave = body.size();
    if (receivedBodyData.size() <= alreadyHave)//marecevina walou
    {
        parsingRequestComplete = false;//need more data
        return;
    }

    const size_t availableNew = receivedBodyData.size() - alreadyHave;
    const size_t remaining = expectedBodyLength - alreadyHave;
    const size_t bytesToCopy = std::min(availableNew, remaining);

    body.append(receivedBodyData, alreadyHave, bytesToCopy);//appendi dkchi jdid
    bodyBytes += bytesToCopy;//update ch7al copina
    std::cout << "\n\n\n body after append: " << body << "\n\n\n" << std::endl; 

    if (body.size() == expectedBodyLength)
        parsingRequestComplete = true;
}


int httpRequest::validateRequest()
{

    if (method != "GET" && method != "POST" && method != "DELETE")
        return (statusCode = 405);

    if (httpVersion != "HTTP/1.1" && httpVersion != "HTTP/1.0")
        return (statusCode = 505);// HTTP Version Not Supported

    if (httpVersion == "HTTP/1.1" && headers.find("host") == headers.end())
        return (statusCode = 400); // host header mkynch

    if (headers.find("host") != headers.end() && headers["host"].empty())
        return (statusCode = 400);

    if (headers.find("content-length") != headers.end() && headers["content-length"].empty())
        return (statusCode = 400);

    if (headers.find("content-type") != headers.end() && headers["content-type"].empty())
        return (statusCode = 400);

    if (headers.find("transfer-encoding") != headers.end())
        return (statusCode = 501);

    if (method == "POST" && headers.find("content-length") == headers.end())
        return (statusCode = 411);

    if (path.empty() || path[0] != '/' || path.find("..") != std::string::npos)
        return (statusCode = 403); // invalid path

    return 0;
}
