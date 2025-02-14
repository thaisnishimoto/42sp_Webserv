#include "Request.hpp"

Request::Request(void)
{
    parsedMethod = false;
    parsedRequestLine = false;
    parsedHeader = false;
    validatedHeader = false;
    parsedBody = false;
    bodyTooLarge = false;
    badRequest = false;
    continueParsing = true;
    contentLength = 0;
    isChunked = false;
	isDir = false;
    isCgi = false;
}

std::string Request::getHeader(std::string fieldName)
{
    if(headerFields.count(fieldName) == 1)
    {
        return headerFields[fieldName];
    }
    return "";
}