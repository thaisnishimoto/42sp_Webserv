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
    void setEnvVars(void);
    std::vector<char *> prepareEnvp(void);

  private:
    Connection& _connection;
    std::string _scriptPath;
    std::vector<std::string> _envVars;
    std::string _outputData;
};

#endif