#include "WebServer.hpp"
#include "Logger.hpp"

Cgi::Cgi(Connection& connection, Location& location)
{
    //After merging
    //_scriptPath = connection.request.localPathName; 
    _scriptPath = "." + location.getRoot() + connection.request.target; 
    std::cout << "Script path: " << _scriptPath << std::endl;
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
        char* const envp[] = {NULL};
        
        execve("/usr/bin/python3", argv, envp);
        std::cerr << "Execve error" << std::endl;
        exit(EXIT_FAILURE);
    }
    //if POST, write body to pipe[1]
    close(pipeFd[1]);

    int status;
    waitpid(pid, &status, 0);

    char buff[101];
    ssize_t bytes = read(pipeFd[0], buff, 100);
    if (bytes > 0)
    {
        // buff[bytes] = '\0';
        std::cout << "Script output: " << buff << std::endl;
    }
    else
    {
        std::cerr << "Error reading from pipe" << std::endl;
    }

    close(pipeFd[0]);
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