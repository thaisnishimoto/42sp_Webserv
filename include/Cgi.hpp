#ifndef _CGI_HPP_
#define _CGI_HPP_

#include "Response.hpp"
#include "Connection.hpp"
#include "utils.hpp"
#include "Logger.hpp"
#include <vector>
#include <sys/wait.h> //waitpid
#include <string> //istringstream
#include <cstdlib> //exit

class Cgi
{
  public:
    Cgi(Connection& connection);
    Connection& _connection;
    int executeScript(void);
    std::string getScriptPath(void) {return _scriptPath;}
    std::string& getOutput(void) {return _rawOutputData;}
    int getPid(void) {return _pid;}
    int getPipeFd(void) {return _pipeFd;}
    void setEnvVars(void);
    std::vector<char *> prepareEnvp(void);
    bool exited;
    time_t lastActivity;
    void handleError(std::string msg);
    void closePipe(int pipeFd[2]);

  private:
    Logger _logger;
    std::string _scriptPath;
    std::string _interpreter;
    std::vector<std::string> _envVars;
    std::string _rawOutputData;
    int _pid;
    int _pipeFd;
};

#endif