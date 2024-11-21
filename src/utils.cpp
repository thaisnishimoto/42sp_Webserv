#include "utils.hpp"

void    tolower(std::string& str)
{
    std::string::iterator it;
    for (it = str.begin(); it != str.end(); ++it)
    {
        std::tolower(*it);
    }
}