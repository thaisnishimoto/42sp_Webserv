#include "Request.hpp"

std::set<std::string> Request::_implementedMethods;
std::set<std::string> Request::_otherMethods;

void Request::initStaticMethods()
{
    Request::_implementedMethods.insert("GET");
    Request::_implementedMethods.insert("POST");
    Request::_implementedMethods.insert("DELETE");

    Request::_otherMethods.insert("HEAD");
    Request::_otherMethods.insert("PUT");
    Request::_otherMethods.insert("CONNECT");
    Request::_otherMethods.insert("OPTIONS");
    Request::_otherMethods.insert("TRACE");
}

void	Request::parseRequest(std::string& buffer)
{
	parseRequestLine(buffer);
}

void	Request::parseRequestLine(std::string& buffer)
{

	_requestLine = getLineRN(buffer);
	if (_requestLine.size() == 0)
	{
		buffer = buffer.substr(2, std::string::npos);
		_requestLine = getLineRN(buffer);
	}

	std::cout << "request line: " << _requestLine << std::endl;
	std::string requestLineCpy = _requestLine;
	parseMethod(requestLineCpy);
}


std::string Request::getLineRN(std::string buffer)
{
    std::string line;
    size_t posRN = buffer.find("\r\n");

    line = buffer.substr(0, posRN);

    return line;
}

void	Request::parseMethod(std::string& requestLine)
{
	_method = requestLine.substr(0, requestLine.find(" "));
	std::cout << "method: " << _method << std::endl;
	if (_implementedMethods.count(_method) == 1)
	{
		_statusCode = OK;
	}
	else if (_otherMethods.count(_method) == 1)
	{
		_statusCode = METHOD_NOT_IMPLEMENTED;
	}
	else
		_statusCode = BAD_REQUEST;
	std::cout << "status code: " << _statusCode << std::endl;

	requestLine = requestLine.substr(_method.size(), std::string::npos);
	std::cout << "Remainder of request line: " << requestLine << std::endl;

	//next method will need to verify if number of whitespaces are adequate
}
