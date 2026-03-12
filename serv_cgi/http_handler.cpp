#include "client_class.hpp"
#include "../parsingRequest/parsing_request.hpp"
#include "../config/config.hpp"


bool isallowed(const client_class& cl, const std::string& method)
{
    std::vector<LocationConfig> locations = cl.getserver().getLocations();
    for (size_t i = 0; i < locations.size(); ++i)
    {
        std::vector<std::string> methods = locations[i].getAllowedMethods();
        if (std::find(methods.begin(), methods.end(), method) != methods.end())
            return true;
    }
    return false;
}


std::string wiche_met(client_class &cl)
{
    httpRequest &req = cl.getrequest();
    std::string response;
    int ret = 0;
    if(!isallowed(cl, req.method))
    {
        req.statusCode = 405;
        response = cl.getresponse().buildError(req.statusCode);
        cl.headerSent = true;
        cl.responseDone = true;  
        return (response);
    }else if(is_cgi(cl.getserver(), req.path))
    {
        cl.is_cgi = true;
        // std::cout << "Handling CGI request for path: " << req.path << std::endl;
        cl.ret = handle_cgi(cl);

    }
    else if(req.method == "GET" )
    {
        // std::cout << "Handling get request for path: " << std::endl;

        ret = req.handleGetRequest(cl.getserver());
    }
    else if(req.method == "POST")
    {

        // std::cout << "Handling post request for path: " << req.path<< std::endl;
        ret = req.handlePostRequest(cl.getserver());
        // cl.responseDone = true;
    }
    else if(req.method == "DELETE")
    {
        // std::cout << "Handling delete request for path: " << std::endl;
        ret = req.handleDeleteRequest(cl.getserver());
        // cl.responseDone = true;
    }

    if (ret != 0 && !cl.is_cgi)
    {
        response = cl.getresponse().buildError(req.statusCode);
        cl.headerSent = true;
        cl.responseDone = true;
    } else if (!cl.is_cgi)
    {
        response = cl.getresponse().buildHeaders(req,req.statusCode);
        cl.headerSent = true;
    }

    return(response);
}


