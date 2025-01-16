#ifndef _REQUEST_HPP_
#define _REQUEST_HPP_

#include <map>
#include <string>

class Request
{
  public:
    std::string method;
    std::string target;
    std::map<std::string, std::string> headerFields;
    std::string body;
    size_t contentLength;

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

    // public:
    //     static void initStaticMethods(void);
    //    void	parseRequest(std::string& buffer);
    // void	parseHeader(std::string& buffer);
    // std::string captureFieldName(std::string& fieldLine);
    // std::string captureFieldValues(std::string& fieldLine);
    // void	validateHeader(void);
    // bool	validateContentLength(void);
    // bool	validateHost(void);
    // bool	findExtraRN(void);
    // std::string removeCRLF(std::string& fieldValue);
};

#endif
