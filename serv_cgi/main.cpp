#include "client_class.hpp"
#include "../parsingRequest/parsing_request.hpp"
#include "../config/config.hpp"



Config globalConfig;


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    try
    {
        globalConfig.parse(argv[1]);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    // globalConfig.printConfig();
    int ret = 0;
    std::vector<std::string> users;
    std::map<std::string,std::string> sessions;
    ret = handelServers(users, sessions);

    return (ret);
}

