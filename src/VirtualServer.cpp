#include "VirtualServer.hpp"

VirtualServer::VirtualServer()
{
    _logger.log(DEBUG, "New VirtualServer created");
    initReferences();
    _default = false;
    _host = 0x7f000001;
    _port = 0x0050;
    _serverName = "";
    setDefaultsErrorsPage();
    _clientMaxBodySize = 1024;
}

void VirtualServer::initReferences()
{
    // Directives
    _refAllowedServerDirective.insert("host");
    _refAllowedServerDirective.insert("port");
    _refAllowedServerDirective.insert("server_name");
    _refAllowedServerDirective.insert("error_page");
    _refAllowedServerDirective.insert("client_max_body_size");

    // Errors
    _refAllowedErrorCode.insert("403");
    _refAllowedErrorCode.insert("404");
    _refAllowedErrorCode.insert("405");
    _refAllowedErrorCode.insert("409");
    _refAllowedErrorCode.insert("413");
    _refAllowedErrorCode.insert("415");
    _refAllowedErrorCode.insert("500");
    _refAllowedErrorCode.insert("501");
}

void VirtualServer::validateDirective(const std::string& directive)
{
    bool isAllowed = false;

    if (_refAllowedServerDirective.find(directive) !=
        _refAllowedServerDirective.end())
    {
        isAllowed = true;
        _refAllowedServerDirective.erase(directive);
    }
    if (isAllowed == false)
    {
        _logger.log(ERROR, "Invalid server directive: '" + directive +
                               "' or directive already defined");
        throw std::runtime_error("");
    }
}

void VirtualServer::setDefault() { _default = true; }

void VirtualServer::validateErrorCode(std::string& code)
{
    bool isAllowed = false;
    if (_refAllowedErrorCode.find(code) != _refAllowedErrorCode.end())
    {
        isAllowed = true;
        _refAllowedErrorCode.erase(code);
    }
    if (isAllowed == false)
    {
        _logger.log(ERROR, "This error is not supported or already was set: '" +
                               code + "'");
        throw std::runtime_error("");
    }
}

Location* VirtualServer::validateFallbackLocation(std::string resource)
{
    if (_locations.find(resource) != _locations.end())
    {
        return &_locations[resource];
    }
    return NULL;
}

void VirtualServer::setDefaultsErrorsPage()
{
    std::string errorPageRoot = "errors/";
    _errorPages["403"] = errorPageRoot + "403.html";
    _errorPages["404"] = errorPageRoot + "404.html";
    _errorPages["405"] = errorPageRoot + "405.html";
    _errorPages["409"] = errorPageRoot + "409.html";
    _errorPages["413"] = errorPageRoot + "413.html";
    _errorPages["415"] = errorPageRoot + "415.html";
    _errorPages["500"] = errorPageRoot + "500.html";
    _errorPages["501"] = errorPageRoot + "501.html";
}

void VirtualServer::setErrorsPage(std::stringstream& serverBlock)
{
    std::string code;
    while (serverBlock >> code)
    {
        validateErrorCode(code);
        std::pair<std::string, std::string> pair;
        pair.first = code;
        std::string path;
        serverBlock >> path;
        if (removeLastChar(path, ';') == true)
        {
            pair.second = path;
            _errorPages[code] = path;
            break;
        }
        pair.second = path;
        _errorPages[code] = path;
    }
}

uint32_t VirtualServer::ipStringToNetOrder(std::string& ip)
{
    Logger _logger;
    unsigned int octets[4] = {0};
    char dots[4];
    bool err = false;
    uint32_t host;
    std::stringstream ss(ip);

    ss >> octets[0] >> dots[0] >> octets[1] >> dots[1] >> octets[2] >>
        dots[2] >> octets[3];
    dots[3] = '.';
    for (int i = 0; i < 4; i++)
    {
        if (octets[i] > 255 || dots[i] != '.')
        {
            err = true;
        }
    }
    if (ss == false || ss.eof() == false || err == true)
    {
        _logger.log(ERROR,
                    "Invalid host value '" + ip +
                        "'. Host must be between 0.0.0.0 - 255.255.255.255");
        throw std::runtime_error("");
    }
    host = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    return host;
}

void VirtualServer::setHost(std::string& directiveValue)
{
    if (directiveValue == "localhost")
    {
        directiveValue = "127.0.0.1";
    }
    _host = ipStringToNetOrder(directiveValue);
}

void VirtualServer::setPort(std::string& directiveValue)
{
    int tmpPort;

    std::stringstream ss(directiveValue);
    if ((ss >> tmpPort) == false || tmpPort < 0 || tmpPort > 65535 ||
        ss.eof() == false)
    {
        _logger.log(ERROR, "Invalid port value '" + directiveValue +
                               "'. Port must be between 0-65635");
        throw std::runtime_error("");
    }
    _port = static_cast<uint16_t>(tmpPort);
};

void VirtualServer::setServerName(std::string& directiveValue)
{
    _serverName = directiveValue;
};

void VirtualServer::setBodySize(std::string& directiveValue)
{
    int tmpBodySize;

    std::stringstream ss(directiveValue);
    if ((ss >> tmpBodySize) == false || tmpBodySize < 1 || tmpBodySize > 1024 ||
        ss.eof() == false)
    {
        _logger.log(ERROR, "Malformed client_max_body_size directive '" +
                               directiveValue + "'");
        throw std::runtime_error("");
    }
    _clientMaxBodySize = static_cast<size_t>(tmpBodySize);
}

void VirtualServer::setLocation(std::pair<std::string, Location>& location)
{
    _locations.insert(location);
}

std::string VirtualServer::getServerName(void) const { return _serverName; }

uint32_t VirtualServer::getHost(void) const { return _host; }

uint16_t VirtualServer::getPort(void) const { return _port; }

size_t VirtualServer::getBodySize(void) const { return _clientMaxBodySize; }

Location* VirtualServer::getLocation(std::string resource)
{
    if (_locations.find(resource) != _locations.end())
    {
        return &_locations[resource];
    }
    return NULL;
}

bool VirtualServer::isStatusCodeError(std::string& code)
{
    if (_errorPages.find(code) != _errorPages.end())
    {
        return true;
    }
    return false;
}

std::string VirtualServer::getErrorPage(std::string errorCode) const
{
	std::string errorPage;
	std::map<std::string, std::string>::const_iterator it;

	it = _errorPages.find(errorCode);

	if (it == _errorPages.end())
	{
		return errorPage;
	}

	errorPage = "./content/";
	errorPage += it->second;
	return errorPage;
}
