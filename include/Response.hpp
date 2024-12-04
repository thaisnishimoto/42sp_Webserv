#ifndef _RESPONSE_HPP_
#define _RESPONSE_HPP_

#include <iostream>
#include <map>

class Response
{
public:
	std::string statusLine;
	std::map<std::string, std::string> headerFields;
	std::string body;

	std::string statusCode;
	std::string reasonPhrase;
};

#endif //_RESPONSE_HPP_
