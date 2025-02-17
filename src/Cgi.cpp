#include "Cgi.hpp"
#include "WebServer.hpp"

Cgi::Cgi(Connection& connection) : _connection(connection)
{
    _scriptPath = connection.request.localPathname; 
    exited = false;
    lastActivity = time(NULL);
}

void Cgi::closePipe(int pipeFd[2])
{
    close(pipeFd[0]);
    close(pipeFd[1]);
}

int Cgi::executeScript(void)
{
    int pipeFd[2];
    if (pipe(pipeFd) == -1)
    {
        handleError("Cgi pipe failed");
        return -1;
    }
    if (!setNonBlocking(pipeFd[0]) || !setNonBlocking(pipeFd[1]))
    {
        closePipe(pipeFd);
        handleError("Could not sef pipe fd to non blocking");
        return -1;
    }
    int pid = fork();
    if (pid == -1)
    {
        handleError("Cgi fork failed");
        return -1;
    }
    
    if (pid == 0)
    {
        if (dup2(pipeFd[0], STDIN_FILENO) == -1)
        {
            closePipe(pipeFd);
            exit(EXIT_FAILURE);
        }
        close(pipeFd[0]);

        if (dup2(pipeFd[1], STDOUT_FILENO) == -1)
        {
            closePipe(pipeFd);
            exit(EXIT_FAILURE);
        }
        close(pipeFd[1]);

        char* const argv[] = {const_cast<char *>("/usr/bin/python3"), const_cast<char *>(_scriptPath.c_str()), NULL};    
        setEnvVars();
        std::vector<char *> envp = prepareEnvp();

        execve("/usr/bin/python3", argv, envp.data());
        closePipe(pipeFd);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        exit(EXIT_FAILURE);
    }
    else
    {
        _pid = pid;
        if (_connection.request.method == "POST")
        {
            if (write(pipeFd[1], _connection.request.body.c_str(), _connection.request.body.size()) == -1)
            {
                closePipe(pipeFd);
                handleError("Writting request body to Cgi Pipe Fd failed");
                return -1;
            }
        }
        close(pipeFd[1]);

        _pipeFd = pipeFd[0];
        return pipeFd[0];
    }
}

void Cgi::handleError(std::string msg)
{
    _logger.log(ERROR, msg);
    _connection.response.setStatusLine("500", "Internal Server Error");
}

void Cgi::setEnvVars(void)
{
    Request& request = _connection.request;
    VirtualServer& virtualServer = *_connection.virtualServer;

    _envVars.push_back("SERVER_NAME=" + virtualServer.getServerName());
    _envVars.push_back("SERVER_PORT=" + itoa(virtualServer.getPort()));
    _envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    _envVars.push_back("SCRIPT_NAME=" + request.target);
    _envVars.push_back("REQUEST_METHOD=" + request.method);
    _envVars.push_back("PATH_INFO=" + request.localPathname);
    if (!request.queryString.empty())
    {
        _envVars.push_back("QUERY_STRING=" + request.queryString);
    }
    if (request.method == "POST")
    {
        _envVars.push_back("CONTENT_LENGTH=" + itoa(request.body.size()));
        std::string contentType = request.getHeader("content-type");
        if (contentType.empty())
            contentType = "application/octet-stream";
        _envVars.push_back("CONTENT_TYPE=" + contentType); 
    }
}

std::vector<char *> Cgi::prepareEnvp(void)
{
    std::vector<char *> envp;
    
    std::vector<std::string>::iterator it = _envVars.begin();
    std::vector<std::string>::iterator ite = _envVars.end();
    while (it != ite)
    {
        envp.push_back(const_cast<char *>(it->c_str())); 
        ++it;
    }
    envp.push_back(NULL); 
    return envp;
}

void WebServer::parseQueryString(std::string& requestTarget, Request& request)
{
    size_t pos = requestTarget.find('?');
    if (pos != std::string::npos)
    {
        request.queryString = requestTarget.substr(pos + 1);
        requestTarget = requestTarget.substr(0, pos);
    }
}

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
                    //maybe add body?
                    return;
                }
                response.setHeader(fieldName, fieldValue);
            }
        }
    }
    if (!response.body.empty() && response.headerFields.count("content-type") == 0)
    {
        response.setStatusLine("500", "Internal Server Error");
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
            // Connection& connection = cgiInstance->_connection;
            _logger.log(INFO, "Cgi Timeout. Pipe Fd: " +
                                itoa(cgiInstance->getPipeFd()));

            if (kill(cgiInstance->getPid(), SIGKILL) == 0)
                _logger.log(DEBUG, "CGI process " + itoa(cgiInstance->getPid()) + " killed.");
            else
                _logger.log(ERROR, "Failed to kill CGI process " + itoa(cgiInstance->getPid()));
            cgiInstance->lastActivity = time(NULL);
            // epoll_ctl(_epollFd, EPOLL_CTL_DEL, cgiInstance->getPipeFd(), NULL);
            // _logger.log(DEBUG, "Pipe Fd " + itoa(cgiInstance->getPipeFd()) +
            //                     " deleted from epoll instance");

            // std::map<int, Cgi*>::iterator temp = it;
            // ++it;
            // _cgiMap.erase(temp);
            // _logger.log(DEBUG, "Cgi instance removed from cgiMap");
            // close(cgiInstance->getPipeFd());

            // connection.response.isWaitingForCgiOutput = false;
            // connection.response.setStatusLine("500", "Internal Server Error");
            // buildResponseBuffer(connection);
            // connection.response.isReady = true;

            // delete cgiInstance;
            // continue;
        }
        ++it;
    }
}
