#include "WebServer.hpp"
#include "Logger.hpp"

void WebServer::parseQueryString(std::string& requestTarget, Request& request)
{
    size_t pos = requestTarget.find('?');
    if (pos != std::string::npos)
    {
        request.queryString = requestTarget.substr(pos + 1);
        // std::cout << "Parsed query string: " << request.queryString << std::endl;
        requestTarget = requestTarget.substr(0, pos);
    }
}

bool WebServer::isCgiRequest(Connection& connection)
{
    std::string target = connection.request.target;
    if (connection.location->cgi == true && target.find("/cgi-bin/") == 0) 
    {
        if (target.find(".php") != std::string::npos || target.find(".py") != std::string::npos)
            return true;
    }
    return false;
}