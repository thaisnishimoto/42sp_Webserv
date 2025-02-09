#include "Cgi.hpp"
#include "WebServer.hpp"

Cgi::Cgi(Connection& connection) : _connection(connection)
{
    _scriptPath = connection.request.localPathname; 
}

void Cgi::execute(void)
{
    int pipeFd[2];
    if (pipe(pipeFd) == -1)
    {
        std::cerr << "Pipe error" << std::endl;
        return;
    }

    int pid = fork();
    if (pid == -1)
    {
        std::cerr << "Fork error" << std::endl;
        return;
    }
    
    if (pid == 0)
    {
        //if POST, read body from pipe[0]
        close(pipeFd[0]);

        dup2(pipeFd[1], STDOUT_FILENO);
        close(pipeFd[1]);

        char* const argv[] = {const_cast<char *>("/usr/bin/python3"), const_cast<char *>(_scriptPath.c_str()), NULL};    
        setEnvVars();
        std::vector<char *> envp = prepareEnvp();

        execve("/usr/bin/python3", argv, envp.data());
        exit(EXIT_FAILURE);
    }
    //if POST, write body to pipe[1]
    close(pipeFd[1]);

    int status;
    //TODO
    //Change so server doesnt block here
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
        //execve failed
        std::cerr << "Execve error" << std::endl;
    }
    else
    {
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(pipeFd[0], buffer, sizeof(buffer))) > 0)
        {
            _outputData.append(buffer, bytesRead);
            std::cout << "cgi output: " << _outputData << std::endl;
        }
        if (bytesRead == -1)
        {
            //read failed
            std::cerr << "Execve error" << std::endl;
        }
    }
    close(pipeFd[0]);
}

void Cgi::setEnvVars(void)
{
    Request& request = _connection.request;
    VirtualServer& virtualServer = *_connection.virtualServer;

    _envVars.push_back("SERVER_NAME=" + virtualServer.getServerName());
    _envVars.push_back("SERVER_PORT=" + itoa(virtualServer.getPort()));
    _envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    _envVars.push_back("SCRIPT_NAME=" + _scriptPath);
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