#include "Config.hpp"
#include "Location.hpp"
#include "VirtualServer.hpp"
#include <stdexcept>

Config::Config(const std::string& configFile) : _logger(DEBUG2)
{
    loadFile(configFile);
    removeComments();
    extractServerBlocks();
    createVirtualServers();
    _logger.log(INFO, "Configuration file: '" + configFile +
                          "' was read with success!");
}

bool Config::isValidLocation(Location& location)
{
   if (location.getRoot().empty())
  {
      return false;
  }
  return true;
}

void Config::validateVirtualServer(VirtualServer& virtualServer)
{
    std::string errors;
    if (_hasHost == false || _hasPort == false)
    {
        errors += "Missing required directives: ";
        if (_hasHost == false)
            errors += "'host' ";

        if (_hasPort == false)
            errors += "'port' ";
    }
    if (virtualServer.validateFallbackLocation("/") == NULL)
    {
        errors += "Missing required root location '/' ";
    }
    if (errors.empty() == false)
    {
        _logger.log(ERROR, errors + "in the virtual server: " + virtualServer.getServerName());
        throw std::runtime_error("");
    }
    std::map<std::string, Location>::iterator it = virtualServer._locations.begin();
    std::map<std::string, Location>::iterator ite = virtualServer._locations.end();
    while (it != ite)
    {
        if (isValidLocation(it->second) == false)
        {
            _logger.log(ERROR, "Missing required root directive in the location: " + it->first + " from virtual server: " + virtualServer.getServerName());
            throw std::runtime_error("");
        }
        it++;
    }
}

void Config::fillLocation(std::stringstream& ss, Location& location)
{
    std::string token;
    ss >> token;
    location.setResource(token);
    _logger.log(DEBUG, "location directive resource = " + token);
    if (ss >> token && token != "{")
    {
        _logger.log(ERROR, "Malformat bracket at location " +
                               location.getResource() + "directive");
        throw std::runtime_error("");
    }
    while (ss >> token && token != "}")
    {
        location.validateDirective(token);
        if (token == "allowed_methods")
        {
            location.setAllowedMethods(ss);
            continue;
        }
        std::string value;
        ss >> value;
        if (removeLastChar(value, ';') == false)
        {
            _logger.log(ERROR, "Missing ';' at end of line to directive: '" +
                                   token + "'");
            throw std::runtime_error("");
        }
        if (token == "root")
        {
            location.setRoot(value);
        }
        if (token == "index")
        {
            location.setIndex(value);
        }
        if (token == "autoindex")
        {
            if (value == "on")
            {
                location.setAutoIndex(true);
            }
        }
        if (token == "cgi")
        {
            if (value == "on")
            {
                location.setCGI(true);
            }
        }
        if (token == "redirect")
        {
            location.setRedirect(value);
        }
        _logger.log(DEBUG, "location directive " + token + " = " + value);
    }
}

void Config::fillVirtualServer(std::string& serverBlock,
                               VirtualServer& VirtualServer)
{
    std::string token;
    std::string value;
    std::stringstream ss(serverBlock);
    ss >> token;
    if (token == "{")
    {
        while (ss >> token && token != "}")
        {
            if (token == "location")
            {
                Location location;
                fillLocation(ss, location);
                std::pair<std::string, Location> pair(location.getResource(),
                                                      location);
                VirtualServer.setLocation(pair);
            }
            else
            {
                VirtualServer.validateDirective(token);
                if (token == "error_page")
                {
                    VirtualServer.setErrorsPage(ss);
                    continue;
                }
                ss >> value;
                if (removeLastChar(value, ';') == false)
                {
                    _logger.log(ERROR,
                                "missing ';' at end of line from directive: '" +
                                    token + "'");
                    throw std::runtime_error("");
                }
                if (token == "host")
                {
                    VirtualServer.setHost(value);
                    _hasHost = true;
                }
                if (token == "port")
                {
                    VirtualServer.setPort(value);
                    _hasPort = true;
                }
                if (token == "server_name")
                {
                    VirtualServer.setServerName(value);
                }
                if (token == "client_max_body_size")
                {
                    VirtualServer.setBodySize(value);
                }
                _logger.log(DEBUG, "directive " + token + " = " + value);
            }
        }
    }
    else
    {
        _logger.log(ERROR, "Wrong set of brackets in conf file");
        throw std::runtime_error("");
    }
}

void Config::createVirtualServers()
{
    std::vector<std::string>::iterator it;
    std::vector<std::string>::iterator ite;
    it = _serversBlock.begin();
    ite = _serversBlock.end();

    while (it != ite)
    {
        VirtualServer vserver;
        _hasHost = false;
        _hasPort = false;
        fillVirtualServer(*it, vserver);

        validateVirtualServer(vserver);

        std::pair<std::string, VirtualServer> pairNameToVirtualServers(
            vserver.getServerName(), vserver);
        std::pair<uint32_t, uint16_t> pairHostToPort(vserver.getHost(),
                                                     vserver.getPort());
        if (_virtualServers.find(pairHostToPort) != _virtualServers.end())
        {
            // add more VirtualServer for a already set host:port
            _virtualServers[pairHostToPort].insert(pairNameToVirtualServers);
        }
        else
        {
            // handle default VirtualServer
            vserver.setDefault();

            // add first VirtualServer for this host:port
            std::map<std::string, VirtualServer> nameToVirtualServer;
            nameToVirtualServer.insert(pairNameToVirtualServers);
            _virtualServers[pairHostToPort] = nameToVirtualServer;

            // setting default server
            _defaultVirtualServers[pairHostToPort] =
                &_virtualServers[pairHostToPort][vserver.getServerName()];
        }
        it++;
    }
}

void Config::extractServerBlocks()
{
    std::string token;
    while (_sFile >> token)
    {
        std::string block;
        if (token == "server")
        {
            int closeBracket = 0;
            int openBracket = 0;
            char c;
            while (_sFile.get(c))
            {
                if (c == '{')
                {
                    openBracket++;
                }
                else if (c == '}')
                {
                    closeBracket++;
                }
                block += c;
                if (openBracket > 0 && openBracket - closeBracket == 0)
                {
                    break;
                }
            }
            if (openBracket - closeBracket != 0)
            {
                _logger.log(ERROR, "Wrong number of bracket in the conf file");
                throw std::runtime_error("");
            }
        }
        else
        {
            _logger.log(ERROR, "Malformed config file");
            throw std::runtime_error("");
        }
        if (block.empty() == false)
        {
            _serversBlock.push_back(block);
            block.clear();
        }
    }
    if (_serversBlock.empty() == true)
    {
        _logger.log(ERROR, "Could not extract virtual servers from file");
        throw std::runtime_error("");
    }
}

void Config::removeComments()
{
    std::stringstream newFile;
    std::string line;

    while (std::getline(_sFile, line))
    {
        std::size_t commentPos = line.find("//");
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
        }
        trim(line, " \t");
        if (line.empty() == false)
        {
            newFile << line << "\n";
        }
    }
    _sFile.str(newFile.str());
    _sFile.clear();
}

void Config::loadFile(const std::string& configFile)
{
    std::ifstream file(configFile.c_str());
    if (isValidExtension(configFile, ".conf") == false)
    {
        _logger.log(ERROR, "Could not open file, wrong extension '" +
                               configFile + "'. Just file '.conf' is accept");
        throw std::runtime_error("");
    }
    if (!file.is_open())
    {
        _logger.log(ERROR, "Could not open config file '" + configFile + "'");
        throw std::runtime_error("");
    }
    _sFile << file.rdbuf();
	file.close();
}

std::map<std::pair<uint32_t, uint16_t>, VirtualServer*>
Config::getDefaultsVirtualServers(void)
{
    return _defaultVirtualServers;
}

std::map<std::pair<uint32_t, uint16_t>, std::map<std::string, VirtualServer> >
Config::getVirtualServers(void)
{
    return _virtualServers;
}
