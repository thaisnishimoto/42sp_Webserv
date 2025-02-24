#include "Response.hpp"

Response::Response(void)
{
	isReady = false;
	closeAfterSend = false;
    isWaitingForCgiOutput = false;
}

void Response::setHeader(std::string fieldName, std::string fieldValue)
{
    headerFields[fieldName] = fieldValue;
}

void Response::setStatusLine(std::string statusCode, std::string reasonPhrase)
{
    this->statusCode = statusCode;
    this->reasonPhrase = reasonPhrase;
    this->statusLine = "HTTP/1.1 " + statusCode + " " + reasonPhrase;
}