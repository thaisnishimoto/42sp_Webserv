#include "utils.hpp"

void tolower(std::string& str)
{
    std::string::iterator it;
    for (it = str.begin(); it != str.end(); ++it)
    {
        *it = std::tolower(*it);
    }
}

std::string getNextLineRN(std::string& buffer)
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

std::string& trim(std::string& str, const std::string delim)
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

std::string removeCRLF(std::string& fieldValue)
{
    size_t posCRLF = fieldValue.find("\r\n");
    if (posCRLF != std::string::npos)
    {
        fieldValue = fieldValue.substr(0, posCRLF);
    }
    return fieldValue;
}

std::string itoa(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

bool removeLastChar(std::string& word, char delimiter)
{
    bool removed = false;
    if (word.empty() == false && word[word.size() - 1] == delimiter)
    {
        word.erase(word.size() - 1);
        removed = true;
    }
    return removed;
}

bool isValidExtension(const std::string& fileName, const std::string& extension)
{
    std::string fileExtension;
    size_t pos = fileName.rfind(".");

    if (pos == std::string::npos || (fileName.length() <= extension.length()))
    {
        return false;
    }
    else
    {
        fileExtension = fileName.substr(pos);
        if (fileExtension != extension)
        {
            return false;
        }
    }
    return true;
}
