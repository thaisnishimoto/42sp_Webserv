#include "utils.hpp"

void    tolower(std::string& str)
{
    std::string::iterator it;
    for (it = str.begin(); it != str.end(); ++it)
    {
        *it = std::tolower(*it);
    }
}

std::string Request::getNextLineRN(std::string& buffer)
{
    std::string line;

    size_t posRN = buffer.find("\r\n");

    if (posRN != std::string::npos)
    {
        line = buffer.substr(0, posRN + 2);
        buffer = buffer.substr(posRN + 2, std::string::npos);
    }
    return line;
}

std::string& Request::trim(std::string& str, const std::string delim)
{
    // Find the first non-delim character
    size_t start = str.find_first_not_of(delim);
    if (start == std::string::npos)
    {
        // The string is all whitespace
        str = "";
        return str;
    }

    // Find the last non-whitespace character
    size_t end = str.find_last_not_of(delim);

    // Create a substring that excludes leading and trailing whitespace
    str = str.substr(start, end - start + 1);
    return str;
}

std::string Request::removeCRLF(std::string& fieldValue)
{
    size_t posCRLF = fieldValue.find("\r\n");
    if (posCRLF != std::string::npos)
    {
        fieldValue = fieldValue.substr(0, posCRLF);
    }
    return fieldValue;
}
