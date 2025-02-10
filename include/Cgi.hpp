#ifndef _CGI_HPP_
#define _CGI_HPP_

#include "Response.hpp"
#include "Connection.hpp"
#include <vector>
#include <sys/wait.h> //waitpid
#include <string> //istringstream

class Cgi
{
  public:
    Cgi(Connection& connection);
    void execute(void);
    std::string getScriptPath(void) {return _scriptPath;}
    std::string& getOutput(void) {return _rawOutputData;}
    void setEnvVars(void);
    std::vector<char *> prepareEnvp(void);

  private:
    Connection& _connection;
    std::string _scriptPath;
    std::vector<std::string> _envVars;
    std::string _rawOutputData;
};

#endif