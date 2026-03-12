#include "client_class.hpp"
#include "../parsingRequest/parsing_request.hpp"
#include "../config/config.hpp"
#define MAX_EVENTS 1024



bool checkHeader(const std::string &headerx, int a)
{
    if(headerx.empty())
    {
        return(true);
    }
    size_t pos = headerx.find(':');
    if (pos == std::string::npos || pos == 0)
    {
        return false;
    }

    std::string key = headerx.substr(0, pos);
    std::string value = headerx.substr(pos + 1);

    for (size_t i = 0; i < key.size(); i++)
    {
        if (key[i] == ' ' || key[i] == '\t')
        {
            return false;
        }
    }

    while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
        value.erase(0, 1);

    if (a == 0 && key != "Status")
    {
        return false;
    }

    return true;
}



struct pollfd create_client(int client_fd, std::map<int, client_class>& read_cl, std::map<int, client_class>& write_cl, const ServerConfig &server)
{
    struct pollfd cp;
    cp.fd = client_fd;
    cp.events = POLLIN;
    cp.revents = 0;
    read_cl.insert(std::make_pair(client_fd, client_class(client_fd, server)));
    write_cl.insert(std::make_pair(client_fd, client_class(client_fd, server)));
    std::cout << "accept fd= " << client_fd << "\n";
    return(cp);
}

int ft_getline(std::string &headerx, client_class& cl, int fd)
{
    int pos;
    int gpos = 0;
    char c;
    headerx.clear();
    while (1)
    {
        pos = read(fd, &c, 1);
        if (pos <= 0)
            return (pos);
        gpos += pos;
        headerx += c;
        if (c == '\n')
            break;
    }
    cl.getrequest().byte_readed+=gpos;
    return (gpos);
}

int handle_cgi(client_class& cl)
{
    std::string har("/tmp/test");
    if (!cl.is_running)
    {
        cl.is_running = true;
        
        har = har + myToString(client_class::hard++);
        cl.file_hard = har;

        if ((cl.getrequest().responseFd = open(har.c_str(),
                O_CREAT | O_RDWR | O_TRUNC, 0777)) < 0)
        {
            perror("open");
            return (1);
        }

        cl.file_cgi = har;

        cl.cgi_pid = fork();
        if (cl.cgi_pid == 0)
        {
            dup2(cl.getrequest().responseFd, STDOUT_FILENO);
            close(cl.getrequest().responseFd);

            std::vector<LocationConfig> locations =
                cl.getserver().getLocations();

            std::string cginterpreter1;
            std::string cgiPath1;

            for (size_t i = 0; i < locations.size(); i++)
            {
                if (locations[i].getPath() == cl.getrequest().path)
                {
                    cginterpreter1 = locations[i].getCgiInterpreter();
                    cgiPath1 = "./" + cl.getserver().getRoot() +"/"+ locations[i].getCgiPath();
                    break;
                }
            }

            char *argv[] = {
                (char*)cginterpreter1.c_str(),
                (char*)cgiPath1.c_str(),
                NULL
            };

            std::string method =
                "REQUEST_METHOD=" + cl.getrequest().method;

            std::string query =
                "QUERY_STRING=" + cl.getrequest().queryString;

            std::string length =
                "CONTENT_LENGTH=" +
                myToString(cl.getrequest().body.size());

            std::string protocol = "SERVER_PROTOCOL=HTTP/1.1";

            char *envp[] = {
                strdup(method.c_str()),
                strdup(query.c_str()),
                strdup(length.c_str()),
                strdup(protocol.c_str()),
                NULL
            };
            if (execve(argv[0], argv, envp) == -1)
            {
                perror("execve");
                exit(127);
            }
        }
    }
    return (0);
}
bool is_cgi(const ServerConfig &server, std::string& path)
{
    std::vector<LocationConfig> locations = server.getLocations();
    std::string locationPath;
    for(size_t i = 0; i < locations.size(); i++)
    {
        if (locations[i].getPath()[0] != '\\')
            locationPath = "\\" + locations[i].getPath();
        else
            locationPath = locations[i].getPath();
        if((locations[i].getPath() == path) && (!locations[i].getCgiPath().empty()))
            return(true);
    }
    return(false);
}
void give_headers(client_class &cl)
{
    std::string response = cl.getresponse().buildCgiHeaders(cl.getrequest());
    if (cl.getrequest().statusCode == 500)
    {
        cl.responseDone = true;
        cl.headerSent = true;
        response = cl.getresponse().buildError(500);
        write(cl.getfd(), response.c_str(), response.size());
    } else
    {
        write(cl.getfd(), response.c_str(), response.size());
    }
    cl.headerSent = true;
}
















