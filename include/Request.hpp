#ifndef _REQUEST_HPP_
#define _REQUEST_HPP_

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <sstream>

typedef enum
{
	OK = 200,
	BAD_REQUEST = 400,
	METHOD_NOT_IMPLEMENTED = 501
} STATUS_CODE;

class	Request
{
private:
	std::string _requestLine;
	std::map<std::string, std::string> _headerFields;
	static std::set<std::string> _otherMethods;
	static std::set<std::string> _implementedMethods;
	std::string _body;

	std::string _method;
	std::string _requestTarget;

	STATUS_CODE _statusCode;

public:
    static void initStaticMethods();
    void	parseRequest(std::string& buffer);
	void	parseRequestLine(std::string& buffer);
	void	parseMethod(std::string& requestLine);
	std::string getLineRN(std::string buffer);
};

#endif
