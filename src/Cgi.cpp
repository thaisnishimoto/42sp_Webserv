#include "Cgi.hpp"
#include "WebServer.hpp"

Cgi::Cgi(Connection& connection) : _connection(connection)
{
    _scriptPath = connection.request.localPathname; 
}

int Cgi::executeScript(void)
{
    int pipeFd[2];
    if (pipe(pipeFd) == -1)
    {
        std::cerr << "Pipe error" << std::endl;
        return -1;
    }
    if (!setNonBlocking(pipeFd[0]) || !setNonBlocking(pipeFd[1]))
    {
        std::cerr << "Server Error: Could not set fd to NonBlocking" << std::endl;
        return -1;
    }

    int pid = fork();
    if (pid == -1)
    {
        std::cerr << "Fork error" << std::endl;
        return -1;
    }
    
    if (pid == 0)
    {
        dup2(pipeFd[0], STDIN_FILENO);
        close(pipeFd[0]);

        dup2(pipeFd[1], STDOUT_FILENO);
        close(pipeFd[1]);

        char* const argv[] = {const_cast<char *>("/usr/bin/python3"), const_cast<char *>(_scriptPath.c_str()), NULL};    
        setEnvVars();
        std::vector<char *> envp = prepareEnvp();

        execve("/usr/bin/python3", argv, envp.data());
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        exit(EXIT_FAILURE);
    }
    else
    {
        _pid = pid;
        if (_connection.request.method == "POST")
            write(pipeFd[1], _connection.request.body.c_str(), _connection.request.body.size());
        close(pipeFd[1]);

        return pipeFd[0];
    }
}

    // int status;
    //TODO
    //Change so server doesnt block here
    // waitpid(pid, &status, 0);
    // if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    // {
    //     //execve failed
    //     std::cerr << "Execve error" << std::endl;
    // }
    // else
    // {
    //     char buffer[1024];
    //     ssize_t bytesRead;
    //     while ((bytesRead = read(pipeFd[0], buffer, sizeof(buffer))) > 0)
    //     {
    //         _rawOutputData.append(buffer, bytesRead);
    //         std::cout << "cgi output: " << _rawOutputData << std::endl;
    //     }
    //     if (bytesRead == -1)
    //     {
    //         //read failed
    //         std::cerr << "Execve error" << std::endl;
    //     }
    // }
    // close(pipeFd[0]);
// }

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
        // std::cout << "Parsed query string: " << request.queryString << std::endl;
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