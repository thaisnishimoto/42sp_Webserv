#ifndef _SEVER_CONFIG_HPP_
#define _SEVER_CONFIG_HPP_

#include "Location.hpp"
#include "Logger.hpp"
#include "VirtualServer.hpp"

#include <fstream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class Config
{
  private:
    std::map<std::pair<uint32_t, uint16_t>,
             std::map<std::string, VirtualServer> >
        _virtualServers;
    std::map<std::pair<uint32_t, uint16_t>, VirtualServer*>
        _defaultVirtualServers;
    std::stringstream _sFile;
    std::vector<std::string> _serversBlock;

    Logger _logger;

    bool _hasHost;
    bool _hasPort;

  public:
    Config(const std::string& configFile);

    void loadFile(const std::string& configFile);
    void removeComments(void);
    void extractServerBlocks(void);
    void createVirtualServers(void);
    void fillVirtualServer(std::string& serverBlock,
                           VirtualServer& VirtualServer);
    void fillLocation(std::stringstream& ss, Location& location);
    void validateVirtualServer(VirtualServer& VirtualServer);
    bool isValidLocation(Location &location);

    // get

    std::map<std::pair<uint32_t, uint16_t>, VirtualServer*>
    getDefaultsVirtualServers(void);
    std::map<std::pair<uint32_t, uint16_t>,
             std::map<std::string, VirtualServer> >
    getVirtualServers(void);
};

#endif /*__SEVER_CONFIG_HPP_*/
