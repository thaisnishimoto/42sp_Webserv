#include "Cgi.hpp"

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

        size_t extPos = _scriptPath.find_last_of('.');
        std::string extension = _scriptPath.substr(extPos);
        if (extension == ".py")
            _interpreter = "/usr/bin/python3";
        else
            _interpreter = "/usr/bin/php";

        char* const argv[] = {const_cast<char *>(_interpreter.c_str()), const_cast<char *>(_scriptPath.c_str()), NULL};    
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