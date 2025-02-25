#include "WebServer.hpp"

bool WebServer::isCgiRequest(Connection& connection, Location& location)
{
    if (location.isCGI() == false)
        return false;

    std::string target = connection.request.target;
    if (target.find("/cgi-bin/") != 0)
        return false;

    size_t extPos = target.find_last_of('.');
    if (extPos != std::string::npos)
    {
        std::string extension = target.substr(extPos);
        if (extension == ".php" || extension == ".py")
            return true;
    }
    return false;
}

void WebServer::buildCgiResponse(Response& response, std::string& cgiOutput)
{
    size_t headerEnd = cgiOutput.find("\n\n");
    if (headerEnd == std::string::npos)
    {
        response.setStatusLine("500", "Internal Server Error");
        response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
        return;
    }

    std::string headers = cgiOutput.substr(0, headerEnd);
    _logger.log(DEBUG, "CGI headers: " + headers);
    response.body = cgiOutput.substr(headerEnd + 2);
    _logger.log(DEBUG, "CGI body: " + response.body);

    std::istringstream headerStream(headers);
    std::string line;
    while (std::getline(headerStream, line))
    {
        if (line.find("Status:") == 0)
            response.statusLine = line.substr(8);
        else
        {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos)
            {
                std::string fieldName = line.substr(0, colonPos);
                tolower(fieldName);
                fieldName = trim(fieldName, " ");

                std::string fieldValue = line.substr(colonPos + 1);
                fieldValue = trim(fieldValue, " ");

                if (response.headerFields.count(fieldName) != 0)
                {
                    response.setStatusLine("500", "Internal Server Error");
                    response.closeAfterSend = true;
                    response.headerFields["connection"] = "close";
                    return;
                }
                response.setHeader(fieldName, fieldValue);
            }
        }
    }
    if (!response.body.empty() && response.headerFields.count("content-type") == 0)
    {
        response.setStatusLine("500", "Internal Server Error");
        response.closeAfterSend = true;
        response.headerFields["connection"] = "close";
        return;
    }
    if (response.statusLine.empty())
        response.setStatusLine("200", "OK");
    if (!response.body.empty() && response.headerFields.count("content-lenght") == 0)
        response.setHeader("content-length", itoa(response.body.size()));
}

void WebServer::checkCgiTimeouts(void)
{
    time_t now = time(NULL);
    std::map<int, Cgi*>::iterator it = _cgiMap.begin();
    std::map<int, Cgi*>::iterator ite = _cgiMap.end();
    while (it != ite)
    {
        Cgi* cgiInstance = it->second;
        if (now - cgiInstance->lastActivity > CGI_TIMEOUT)
        {
            _logger.log(INFO, "Cgi Timeout. Pipe Fd: " +
                                itoa(cgiInstance->getPipeFd()));

            if (kill(cgiInstance->getPid(), SIGKILL) == 0)
                _logger.log(DEBUG, "CGI process " + itoa(cgiInstance->getPid()) + " killed.");
            else
                _logger.log(ERROR, "Failed to kill CGI process " + itoa(cgiInstance->getPid()));
            cgiInstance->lastActivity = time(NULL);
        }
        ++it;
    }
}

void WebServer::registerCgiPipe(int pipeFd, Cgi* cgiInstance, uint32_t events)
{
    Response& response = cgiInstance->_connection.response;

    struct epoll_event cgiEvent;
    std::memset(&cgiEvent, 0, sizeof(cgiEvent));
    cgiEvent.events = events;
    cgiEvent.data.fd = pipeFd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, pipeFd, &cgiEvent) == -1)
    {
        std::string msg = "Failed to add to Epoll instance. Fd: " + itoa(pipeFd);
        _logger.log(ERROR, msg);
        response.setStatusLine("500", "Internal Server Error");
        response.closeAfterSend = true;
		response.headerFields["connection"] = "close";

        int cgiPid = cgiInstance->getPid();
        if (kill(cgiPid, SIGKILL) == 0)
        {
            _logger.log(DEBUG, "Sent SIGKILL to CGI process: " + itoa(cgiPid));
        }
        else
        {
            _logger.log(ERROR, "Failed to send SIGKILL to CGI process: " +
                                itoa(cgiPid) + " - " + std::strerror(errno));
        }
        while (waitpid(cgiPid, NULL, WNOHANG) > 0);
        _logger.log(DEBUG, "Cgi process reaped: " + itoa(cgiPid));

        close(pipeFd);
        delete cgiInstance;
        return;
    }
    _logger.log(DEBUG,
                "Cgi pipe Fd " + itoa(pipeFd) + " added to epoll instance - " +
                ((events & EPOLLIN) ? "EPOLLIN" : "EPOLLOUT"));
    if (_cgiMap.count(pipeFd) != 0)
    {
        std::string msg = "Pipe fd already listed as cgi instance in server. Fd: " + itoa(pipeFd);
        _logger.log(ERROR, msg);
        response.setStatusLine("500", "Internal Server Error");
        response.closeAfterSend = true;
		response.headerFields["connection"] = "close";

        int cgiPid = cgiInstance->getPid();
        if (kill(cgiPid, SIGKILL) == 0)
        {
            _logger.log(DEBUG, "Sent SIGKILL to CGI process: " + itoa(cgiPid));
        }
        else
        {
            _logger.log(ERROR, "Failed to send SIGKILL to CGI process: " +
                                itoa(cgiPid) + " - " + std::strerror(errno));
        }
        while (waitpid(cgiPid, NULL, WNOHANG) > 0);
        _logger.log(DEBUG, "Cgi process reaped: " + itoa(cgiPid));

        if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, pipeFd, &cgiEvent) == 0)
        {
            _logger.log(DEBUG, "Pipe Fd " + itoa(pipeFd) +
                                " deleted from epoll instance");
        }

        close(pipeFd);
        delete cgiInstance;
        return;
    }
    _cgiMap[pipeFd] = cgiInstance;
    _logger.log(DEBUG,
                "cgiInstance added to cgiMap. Pipe Fd: " + itoa(pipeFd));
    response.isWaitingForCgiOutput = true;
}