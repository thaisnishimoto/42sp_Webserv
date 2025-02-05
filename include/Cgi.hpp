#ifndef _CGI_HPP_
#define _CGI_HPP_

#include "Response.hpp"
#include "Connection.hpp"
#include <vector>
#include <sys/wait.h> //waitpid

class Cgi
{
  public:
    Cgi(Connection& connection);
    void execute(void);
    // Response& getHTTPResponse(void); 
    std::string getScriptPath(void) {return _scriptPath;}

  private:
    std::string _scriptPath;
    // std::string _rawOutput;
    // std::vector<char *> _envp;
    // void _setEnvironmentVariables(Connection& connection);
};

#endif