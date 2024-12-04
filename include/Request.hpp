#ifndef _REQUEST_HPP_
#define _REQUEST_HPP_

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <sstream>

// typedef enum
// {
// 	OK = 200,
// 	BAD_REQUEST = 400,
// 	METHOD_NOT_IMPLEMENTED = 501
// } STATUS_CODE;

class	Request
{
public:
	std::string requestLine;
	std::map<std::string, std::string> headerFields;
	std::string body;

	std::string method;
	std::string target;

	bool parsedMethod;
	bool parsedRequestLine;
	bool parsedHeader;
	bool parsedBody;

	bool badRequest;
	
	bool continueParsing;

	Request(void);

// public:
//     static void initStaticMethods(void);
    void	parseRequest(std::string& buffer);
	void	parseRequestLine(std::string& buffer);
	void	parseMethod(std::string& requestLine);
	void	parseTarget(std::string& requestLine);
	void	parseVersion(std::string& requestLine);
	void	parseHeader(std::string& buffer);
	std::string captureFieldName(std::string& fieldLine);
	std::string captureFieldValues(std::string& fieldLine);
	void	validateHeader(void);
	bool	validateContentLength(void);
	bool	validateHost(void);
	bool	findExtraRN(void);
	std::string removeCRLF(std::string& fieldValue);
};

#endif
