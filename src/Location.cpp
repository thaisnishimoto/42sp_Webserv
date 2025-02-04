#include "Location.hpp"

Location::Location(void)
{
    _logger.log(DEBUG, "New Location created");
    initReferences();
    _index = "index.html";
    _resource = "";
    _root = "";
    _autoIndex = false;
    _redirect = "";
    _cgi = false;
}

void Location::initReferences()
{
    // Directives
    _refAllowedLocationDirective.insert("root");
    _refAllowedLocationDirective.insert("redirect");
    _refAllowedLocationDirective.insert("autoindex");
    _refAllowedLocationDirective.insert("allowed_methods");
    _refAllowedLocationDirective.insert("cgi");
    _refAllowedLocationDirective.insert("index");

    // methods
    _refAllowedMethods.insert("GET");
    _refAllowedMethods.insert("POST");
    _refAllowedMethods.insert("DELETE");
    _refAllowedMethods.insert("HEAD");
    _refAllowedMethods.insert("PUT");
    _refAllowedMethods.insert("CONNECT");
    _refAllowedMethods.insert("OPTIONS");
    _refAllowedMethods.insert("TRACE");
}

void Location::validateDirective(const std::string& directive)
{
    bool isAllowed = false;

    if (_refAllowedLocationDirective.find(directive) !=
        _refAllowedLocationDirective.end())
    {
        isAllowed = true;
        _refAllowedLocationDirective.erase(directive);
    }
    if (isAllowed == false)
    {
        _logger.log(ERROR, "Invalid location directive: '" + directive +
                               "' or directive already defined");
        throw std::runtime_error("");
    }
}

void Location::validateMethod(const std::string& method)
{
    bool isAllowed = false;
    if (_refAllowedMethods.find(method) != _refAllowedMethods.end())
    {
        isAllowed = true;
        _refAllowedMethods.erase(method);
    }

    if (isAllowed == false)
    {
        _logger.log(ERROR, "HTTP method: '" + method +
                               "' is invalid or already cofigured");
        throw std::runtime_error("");
    }
}

void Location::setResource(std::string& path) { _resource = path; }

void Location::setAllowedMethods(std::stringstream& serverBlock)
{
    std::string value;
    _logger.log(DEBUG, "location directive allowed_method: ");
    while (serverBlock >> value)
    {
        if (removeLastChar(value, ';') == true)
        {
            validateMethod(value);
            _allowedMethods.insert(value);
            std::cout << "\033[33m" << value << std::endl;
            break;
        }
        validateMethod(value);
        _allowedMethods.insert(value);
        std::cout << "\033[33m" << value << ", ";
    }
    if (_allowedMethods.empty() == true)
    {
        _logger.log(ERROR,
                    "No HTTP method provide to directive: allowed_method");
        throw std::runtime_error("");
    }
}

void Location::setIndex(std::string& file) { _index = file; }

void Location::setRoot(std::string& path) { _root = path; }

void Location::setAutoIndex(bool value) { _autoIndex = value; }

void Location::setRedirect(std::string& path) { _redirect = path; }

void Location::setCGI(bool value) { _cgi = value; }

// gets
std::string Location::getResource(void) const { return _resource; }

std::string Location::getRoot(void) const { return _root; }

std::string Location::getRedirect(void) const { return _redirect; }

std::string Location::getIndex(void) const { return _index; }

std::string Location::getAllowedMethods(void) const
{
    std::string list;
    std::set<std::string>::iterator it = _allowedMethods.begin();
    std::set<std::string>::iterator ite = _allowedMethods.end();

    if (_allowedMethods.empty() == true)
    {
        return list;
    }

    if (_allowedMethods.size() == 1)
    {
        list = *it;
        return list;
    }

    list = *it;
    ++it;
    while (it != ite)
    {
        list += ", " + *it;
        ++it;
    }
    return list;
}

// verifiers
bool Location::isCGI(void) const { return _cgi; }

bool Location::isAllowedMethod(const std::string& method)
{
    bool isAllowed = false;

    std::set<std::string>::iterator it = _allowedMethods.begin();
    std::set<std::string>::iterator ite = _allowedMethods.end();
    while (it != ite)
    {
        if (*it == method)
        {
            isAllowed = true;
            break;
        }
        it++;
    }

    return isAllowed;
}

bool Location::isAutoIndex(void) const { return _autoIndex; }
