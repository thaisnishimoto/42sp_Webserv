#ifndef _VIRTUALSERVER_HPP_
#define _VIRTUALSERVER_HPP_

#include "Location.hpp"
#include "Logger.hpp"

#include <map>
#include <netdb.h> //uint16_t
#include <set>
#include <sstream>

class VirtualServer
{
  private:
    bool _default;
    uint32_t _host;
    uint16_t _port;
    std::string _serverName;
    std::map<std::string, std::string> _errorsPage;
    int _clientMaxBodySize;
    std::map<std::string, Location> _locations;

    std::set<std::string> _refAllowedErrorCode;
    std::set<std::string> _refAllowedServerDirective;

    uint32_t ipStringToNetOrder(std::string& ip);
    Logger _logger;

  public:
    VirtualServer();

    // sets
    void setDefaultsErrorsPage();
    void setDefault();
    void setErrorsPage(std::stringstream& serverBlock);
    void setHost(std::string& directiveValue);
    void setPort(std::string& directiveValue);
    void setServerName(std::string& directiveValue);
    void setBodySize(std::string& directiveValue);
    void setLocation(std::pair<std::string, Location>& location);

    // gets
    std::string getServerName(void) const;
    uint32_t getHost(void) const;
    uint16_t getPort(void) const;
    int getBodySize(void) const;
    Location* getLocation(std::string resource);

    // other
    void validateErrorCode(std::string& code);
    void validateDirective(const std::string& directive);
    void initReferences();
};

#endif // _VSERVER_HPP_
