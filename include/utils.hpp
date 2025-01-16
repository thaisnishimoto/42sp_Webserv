#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <cctype>
#include <string>

void tolower(std::string& str);
std::string getNextLineRN(std::string& buffer);
std::string& trim(std::string& str, const std::string delim);
std::string removeCRLF(std::string& fieldValue);
std::string itoa(int n);

#endif
