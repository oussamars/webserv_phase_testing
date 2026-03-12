#include "../parsingRequest/parsing_request.hpp"
#include "../Response/contentType.hpp"

#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

//  POST l "/a/b/file" ktfaili 7it "./uploads/a/b" doesn't exist hna kngado lmouchkil ou ancreew l folder bech nodro naccessiw ldak lfle
static bool mkdirs(const std::string& dir, mode_t mode)
{
    if (dir.empty())
        return false;

    struct stat st;
    if (stat(dir.c_str(), &st) == 0) // if directory deja kayna returni true
        return S_ISDIR(st.st_mode);

    //create parrent directory expl ./uploads/a
    size_t slash = dir.find_last_of('/');
    if (slash != std::string::npos && slash > 0)
    {
        std::string parent = dir.substr(0, slash);
        if (!mkdirs(parent, mode))
            return false;
    }

    //create current directory example b/
    if (mkdir(dir.c_str(), mode) == 0)
        return true;

    if (errno == EEXIST)
    {
        if (stat(dir.c_str(), &st) == 0)
            return S_ISDIR(st.st_mode);
    }
    return false;
}

// returni correct http status bech machi ayi filesystem error dima treturni 500
static int errnoToHttpStatus(int e)
{
    if (e == EACCES || e == EPERM || e == EROFS)
        return 403; // Forbidden
    if (e == ENOSPC)
        return 507; // Insufficient Storage
    return 500;
}

// generate filename mli client posts l "/" oula kyssali b "/"
//bech mytopenach folder 3la anaho file
static std::string generateUploadName(const std::string& contentType)
{
    static unsigned long counter = 0;
    std::string ext = type::getExtension(contentType);
    std::ostringstream oss;
    oss << "upload_" << static_cast<long long>(std::time(NULL))
        << "_" << static_cast<long long>(getpid())
        << "_" << counter++
        << ext;
    return oss.str();
}

int httpRequest::handlePostRequest(const ServerConfig& srv)
{
    if (!parsingRequestComplete)
    {
        this->statusCode = 400;
        return 1;
    }

    if (hasPathTraversal(path))
    {
        this->statusCode = 400;// bad request
        return 1;
    }

    std::map<std::string, std::string>::iterator it = headers.find("content-length");
    if (it == headers.end() || it->second.empty())
    {
        this->statusCode = 411; //length required
        return 1;
    }

    long long contentLength = 0;
    {
        std::istringstream iss(it->second);
        char extra = '\0';
        if (!(iss >> contentLength) || (iss >> extra) || contentLength < 0)
        {
            this->statusCode = 400;
            return 1;
        }
    }

    if (contentLength > static_cast<long long>(MAX_BODY_SIZE))
    {
        this->statusCode = 413; // too large
        return 1;
    }

    if (static_cast<size_t>(contentLength) != body.size())
    {
        this->statusCode = 400;
        return 1;
    }

    std::string baseDir = "." + srv.getRoot();
    if (!mkdirs(baseDir, 0755))
    {
        this->statusCode = errnoToHttpStatus(errno);
        return 1;
    }

    if (path.empty() || path[0] != '/')
    {
        this->statusCode = 400;
        return 1;
    }

    // genere chi filename ila makaynch
    std::string rel = path;
    if (rel == "/" || rel[rel.size() - 1] == '/')
    {
        std::string ct;
        std::map<std::string,std::string>::iterator ctit = headers.find("content-type");
        if (ctit != headers.end())
            ct = ctit->second;
        rel += generateUploadName(ct);
    }

    std::string targetFilePath = baseDir + rel;
    {
        size_t lastSlash = targetFilePath.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            std::string dirOnly = targetFilePath.substr(0, lastSlash);
            if (!mkdirs(dirOnly, 0755))
            {
                this->statusCode = errnoToHttpStatus(errno);
                return 1;
            }
        }
    }

    // ila kant target directory
    bool existed = false;
    {
        struct stat st;
        if (stat(targetFilePath.c_str(), &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                this->statusCode = 409; // conflict
                return 1;
            }
            existed = true;
        }
    }

    std::ofstream outputFile(targetFilePath.c_str(), std::ios::binary | std::ios::trunc);
    if (!outputFile)
    {
        this->statusCode = errnoToHttpStatus(errno);
        return 1;
    }

    if (!body.empty())
        outputFile.write(body.data(), static_cast<std::streamsize>(body.size()));

    if (!outputFile.good())
    {
        this->statusCode = 500;
        return 1;
    }
    outputFile.close();
    
    this->statusCode = existed ? 200 : 201;//200 -> file deja kan mcree // 201 file ylh tcrea jdid
    return 0;
}
