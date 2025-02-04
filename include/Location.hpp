#ifndef _LOCATION_HPP_
#define _LOCATION_HPP_

#include "Logger.hpp"
#include "utils.hpp"

#include <set>
#include <sstream>

class Location
{
  private:
    std::string _resource;
    std::set<std::string> _allowedMethods;
    std::string _root;
    bool _autoIndex;
    std::string _redirect;
    bool _cgi;

    std::set<std::string> _refAllowedLocationDirective;
    std::set<std::string> _refAllowedMethods;

    Logger _logger;

  public:
    Location(void);

    // sets
    void setResource(std::string& path);
    void setAllowedMethods(std::stringstream& method);
    void setRoot(std::string& path);
    void setAutoIndex(bool value);
    void setRedirect(std::string& path);
    void setCGI(bool value);

    // gets
    std::string getResource(void) const;
    std::string getRoot(void) const;
    std::string getRedirect(void) const;
	std::string getAllowedMethods(void) const;

    // verifiers
    bool isCGI(void) const;
    bool isAllowedMethod(const std::string& method);
    bool isAutoIndex(void) const;

    // others
    void initReferences();
    void validateDirective(const std::string& directive);
    void validateMethod(const std::string& method);
};

#endif // _LOCATION_HPP_
