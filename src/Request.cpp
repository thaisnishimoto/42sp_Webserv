#include "Request.hpp"
#include <sstream>

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
	std::cout << "---------------------------------" << std::endl;
	parseRequestLine(buffer);
	std::cout << "status code: " << _statusCode << std::endl;
	if (_statusCode != OK)
		return ;
	parseHeader(buffer);
	std::cout << "status code: " << _statusCode << std::endl;
	std::cout << std::endl;

	std::map<std::string, std::string>::iterator it, ite;
	it = _headerFields.begin();
	ite = _headerFields.end();
	while (it != ite)
	{
		std::cout << "key: " << it->first << " | value: " << it->second << std::endl;
		++it;
	}
	std::cout << "---------------------------------" << std::endl;
}

void	Request::parseRequestLine(std::string& buffer)
{
	_requestLine = getNextLineRN(buffer);
	if (_requestLine.size() == 2)
	{
	   _requestLine = getNextLineRN(buffer);
	}
	std::cout << "request line: " << _requestLine << std::endl;
	std::string requestLineCpy = _requestLine;
	parseMethod(requestLineCpy);
	if (_statusCode != OK)
		return ;
	parseTarget(requestLineCpy);
	if (_statusCode != OK)
		return ;
	parseVersion(requestLineCpy);
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
	{
		_statusCode = BAD_REQUEST;
	}
	std::cout << "status code after parse method: " << _statusCode << std::endl;

	requestLine = requestLine.substr(_method.size() + 1, std::string::npos);
	std::cout << "Remainder of request line: " << "'" << requestLine << "'" << std::endl;

	//next method will need to verify if number of whitespaces are adequate
}

void Request::parseTarget(std::string &requestLine)
{
	_requestTarget = requestLine.substr(0, requestLine.find(" "));
	std::cout << "request target: " << _requestTarget << std::endl;
	if (_requestTarget.size() == 0)
	{
		_statusCode = BAD_REQUEST;
	}
	requestLine = requestLine.substr(_requestTarget.size() + 1, std::string::npos);
	std::cout << "Remainder of request line: " << "'" << requestLine << "'" << std::endl;
}

void Request::parseVersion(std::string &requestLine)
{
    if (requestLine != "HTTP/1.1\r\n")
	{
		_statusCode = BAD_REQUEST;
	}
}

void	Request::parseHeader(std::string& buffer)
{
	std::cout << "inside parseHeader" << std::endl;
	std::cout << "buffer: " << buffer << std::endl;
	std::string headerString = getNextLineRN(buffer);

	while(headerString != "\r\n")
	{
		std::string fieldName;
		std::string fieldValue;
		size_t colonPos = headerString.find(":");

		if (colonPos == std::string::npos)
		{
			_statusCode = BAD_REQUEST;
			return;
		}

		fieldName = headerString.substr(0, colonPos);
		if (fieldName.find(" ") != std::string::npos)
		{
			_statusCode = BAD_REQUEST;
			return;
		}

		std::istringstream stream(headerString.substr(colonPos+1, headerString.size() - 2));
		std::getline(stream, fieldValue);

		std::cout << "Field-name: " << fieldName << std::endl;
		std::cout << "Field-value: " << fieldValue << std::endl;

		std::pair<std::string, std::string> tmp(fieldName, fieldValue);
		_headerFields.insert(tmp);

		headerString = getNextLineRN(buffer);
	}
}
