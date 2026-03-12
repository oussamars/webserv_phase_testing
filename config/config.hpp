#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>


template <typename T> std::string myToString (T value)
{
    std::ostringstream os ;
    os << value ;
    return os.str() ;
}

class LocationConfig {
	private:
		std::string					path; // URL path for this location, example: "/images"
		std::vector<std::string>	allowed_methods; // (allowed methods) list of allowed HTTP methods for this location
		// std::string					root; // overrides server root for this location, if path == "/images", and root == "/var/www/images", and the request is for "/images/pic.jpg", the server will look for the file at "/var/www/images/pic.jpg"
		std::string					index; // (default index file) meaning file to serve when a directory is requested, default is "index.html"
		bool						autoindex; // (autoindex) whether to enable directory listing, default is false
		std::string					cgi_path; // (cgi path) path to the CGI script
		std::string				    cgi_interpreter; // if cgi_path is not empty, the server will execute the CGI script located at
		std::string					redirect; // (redirect) URL to redirect to
	public:

		LocationConfig() : index(""), autoindex(false), cgi_path(""), redirect("") {};

		// setters
		void	setPath (const std::string& path) { this->path = path; };
		void	setAllowedMethods (const std::vector<std::string>& methods) { this->allowed_methods = methods; };
		// void	setRoot (const std::string& root) { this->root = root; };
		void	setIndex (const std::string& index) { this->index = index; };
		void	setAutoindex (bool autoindex) { this->autoindex = autoindex; };
		void	setCgiPath (const std::string& cgi_path) { this->cgi_path = cgi_path; };
		void	setCgiInterpreter (const std::string& cgi_interpreter) { this->cgi_interpreter = cgi_interpreter; };
		void	setRedirect (const std::string& redirect) { this->redirect = redirect; };

		// getters
		const std::string&			getPath () const { return this->path; };
		const std::vector<std::string>&	getAllowedMethods () const { return this->allowed_methods; };
		// const std::string&			getRoot () const { return this->root; };
		const std::string&			getIndex () const { return this->index; };
		bool						isAutoindex () const { return this->autoindex; };
		const std::string&			getCgiPath () const { return this->cgi_path; };
		const std::string&			getCgiInterpreter () const { return this->cgi_interpreter; };
		const std::string&			getRedirect () const { return this->redirect; };
};


class ServerConfig {
	private:
		int							port; // default port is 80 in http
		// std::string					host; // default host is 0.0.0.0
		std::string					server_name; // (domain name) if not set in config, default is "", meaning catch all
		std::string					root; // (default root) where the server will look for files, default is "../app"
		std::string					index; // (default index file) meaning file to serve when a directory is requested, default is "../app/index.html"
		size_t						client_max_body_size; // (default max body size) meaning maximum allowed size for client request body, default is 1MB
		std::map<int, std::string>	error_pages; // (custom error pages)
		std::vector<LocationConfig>	locations; // (location blocks) A location overrides server behavior for a specific URL path.
	public:

		ServerConfig() : port (-1) , index("index.html"), client_max_body_size(1 * 1024 * 1024) {};

		// setters
		void	setPort (int port) { this->port = port; };
		// void	setHost (const std::string& host) { this->host = host; };
		void	setServerName (const std::string& server_name) { this->server_name = server_name; };
		void	setRoot (const std::string& root) { this->root = root; };
		void	setIndex (const std::string& index) { this->index = index; };
		void	setClientMaxBodySize (size_t size) { this->client_max_body_size = size; };
		void	addErrorPage (int code, const std::string& path) { this->error_pages[code] = path; };
		void	addLocation (const LocationConfig& location) { this->locations.push_back(location);};

		// getters
		int					getPort () const { return this->port; };
		// const std::string&	getHost () const { return this->host; };
		const std::string&	getServerName () const { return this->server_name; };
		const std::string&	getRoot () const { return this->root; };
		const std::string&	getIndex () const { return this->index; };
		size_t				getClientMaxBodySize () const { return this->client_max_body_size; };
		const std::map<int, std::string>&	getErrorPages () const { return this->error_pages; };
		const std::vector<LocationConfig>&	getLocations () const { return this->locations; };

		// set root and index for all locations that don't have it set
		void	setDefaultRootAndIndexForLocations();
};


class Config
{
    private:
        std::vector<ServerConfig> servers; // the servers
    public:
        void addServer(const ServerConfig& server) { servers.push_back(server); };
        const std::vector<ServerConfig>& getServers() const { return this->servers; };
        void parse(const std::string& filepath);
        void printConfig() const; // For debugging
};

std::string pathWithOneSlash(const std::string& path);

extern Config globalConfig;