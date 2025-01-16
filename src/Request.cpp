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
}
