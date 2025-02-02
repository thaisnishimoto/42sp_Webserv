#ifndef _CGI_HPP_
#define _CGI_HPP_

#include "Response.hpp"
#include "Connection.hpp"
#include <vector>

class Cgi
{
  public:
    Cgi(Connection& connection, Location& location);
    // std::string execute(void);
    // Response& getHTTPResponse(void); 

  private:
    std::string _scriptPath;
    // std::string _rawOutput;
    // std::vector<char *> _envp;
    // void _setEnvironmentVariables(Connection& connection);
};

#endif