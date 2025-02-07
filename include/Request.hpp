#ifndef _REQUEST_HPP_
#define _REQUEST_HPP_

#include <map>
#include <string>

class Request
{
  public:
    std::string method;
    std::string target;
    std::string queryString;
    std::map<std::string, std::string> headerFields;
    std::string body;
    size_t contentLength;

	std::string locationName;

	bool isDir;
	std::string localPathname;

    bool parsedMethod;
    bool parsedRequestLine;
    bool parsedHeader;
    bool parsedBody;

    bool validatedHeader;

    bool badRequest;
    bool bodyTooLarge;

    bool continueParsing;

    bool isChunked;

    Request(void);
    std::string getHeader(std::string fieldName);

};

#endif
