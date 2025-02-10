#include "Response.hpp"

Response::Response(void)
{
	isReady = false;
	closeAfterSend = false;
}

void Response::setHeader(std::string fieldName, std::string fieldValue)
{
    headerFields[fieldName] = fieldValue;
}

void Response::setStatusLine(std::string statusCode, std::string reasonPhrase)
{
    statusLine = "HTTP/1.1 " + statusCode + " " + reasonPhrase;
}