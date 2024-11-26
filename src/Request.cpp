#include "Request.hpp"
#include "utils.hpp"

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

void Request::parseRequest(std::string& buffer)
{
    std::cout << "---------------------------------" << std::endl;
    parseRequestLine(buffer);
    std::cout << "status code: " << _statusCode << std::endl;
    if (_statusCode != OK)
        return;
    parseHeader(buffer);
    validateHeader();

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

void Request::parseRequestLine(std::string& buffer)
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
        return;
    parseTarget(requestLineCpy);
    if (_statusCode != OK)
        return;
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

void Request::parseMethod(std::string& requestLine)
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

void Request::parseTarget(std::string& requestLine)
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

void Request::parseVersion(std::string& requestLine)
{
    if (requestLine != "HTTP/1.1\r\n")
    {
        _statusCode = BAD_REQUEST;
    }
}

std::string& Request::trim(std::string& str, const std::string delim)
{
    // Find the first non-delim character
    size_t start = str.find_first_not_of(delim);
    if (start == std::string::npos)
    {
        // The string is all whitespace
        str = "";
        return str;
    }

    // Find the last non-whitespace character
    size_t end = str.find_last_not_of(delim);

    // Create a substring that excludes leading and trailing whitespace
    str = str.substr(start, end - start + 1);
    return str;
}

std::string Request::removeCRLF(std::string& fieldValue)
{
    size_t posCRLF = fieldValue.find("\r\n");
    if (posCRLF != std::string::npos)
    {
        fieldValue = fieldValue.substr(0, posCRLF);
    }
    return fieldValue;
}

std::string Request::captureFieldName(std::string& fieldLine)
{
    size_t colonPos = fieldLine.find(":");

    if (colonPos == std::string::npos)
    {
        _statusCode = BAD_REQUEST;
        return "";
    }

    std::string fieldName;
    fieldName = fieldLine.substr(0, colonPos);
    if (fieldName.find(" ") != std::string::npos)
    {
        _statusCode = BAD_REQUEST;
        return "";
    }

    return fieldName;
}

std::string Request::captureFieldValues(std::string& fieldLine)
{
    size_t colonPos = fieldLine.find(":");

    std::string fieldLineTail;
    fieldLineTail = fieldLine.substr(colonPos + 1, std::string::npos);
    std::cout << "FieldLine Tail: " << fieldLineTail << std::endl;

    std::string fieldValues;
    while (true)
    {
        size_t commaPos = fieldLineTail.find(",");
        std::string tmp;
        if (commaPos == std::string::npos)
        {
            // NOTE: removing \r\n from field line
            fieldValues += trim(fieldLineTail, " \t");
            fieldValues = removeCRLF(fieldValues);
            break;
        }
        tmp = fieldLineTail.substr(0, commaPos);
        std::cout << "Pre trim tmp: " << tmp << std::endl;
        tmp = trim(tmp, " \t") + ", ";
        std::cout << "Post trim tmp: " << tmp << std::endl;
        fieldValues += tmp;
        fieldLineTail = fieldLineTail.substr(commaPos + 1, std::string::npos);
        std::cout << "fieldLineTail: " << fieldLineTail << std::endl;
    }

return fieldValues;
}

bool Request::findExtraRN()
{
    std::map<std::string, std::string>::iterator it, ite;
    it = _headerFields.begin();
    ite = _headerFields.end();
    while (it != ite)
    {
        if ((it->first.find('\r') != std::string::npos || it->first.find('\n') != std::string::npos) ||
            (it->second.find('\n') != std::string::npos || it->second.find('\r') != std::string::npos))
        {
            return true;
        }
        ++it;
    }
    return false;
}

void Request::parseHeader(std::string& buffer)
{
    std::cout << "inside parseHeader" << std::endl;
    std::cout << "buffer: " << buffer << std::endl;
    std::string fieldLine = getNextLineRN(buffer);

    while (fieldLine != "\r\n")
    {
        std::string fieldName = captureFieldName(fieldLine);
        if (fieldName.empty() == true)
        {
            return;
        }

        std::string fieldValues = captureFieldValues(fieldLine);

        std::cout << "Field-name: " << fieldName << std::endl;
        std::cout << "Field-value: " << fieldValues << std::endl;

		tolower(fieldName);
        std::pair<std::string, std::string> tmp(fieldName, fieldValues);
        std::pair<std::map<std::string, std::string>::iterator, bool> insertCheck;
        insertCheck = _headerFields.insert(tmp);
        if (insertCheck.second == false)
        {
            _headerFields[fieldName] = _headerFields[fieldName] + ", " + fieldValues;
        }
        fieldLine = getNextLineRN(buffer);
    }
}

bool Request::validateContentLength(void)
{
     if (_headerFields.count("content-length") == 0)
	 {
     	return true;
     }
     if (_headerFields["content-length"].find(",") != std::string::npos ||
         _headerFields["content-length"].find(" ") != std::string::npos)
     {
     	return  false;
     }
     std::istringstream ss(_headerFields["content-length"]);
     long long value;
     ss >> value;
     if (ss.fail() || !ss.eof() || ss.bad() || value < 0)
     {
       return false;
     }
     return true;
}

bool Request::validateHost()
{
    if (_headerFields.count("host") != 1)
    {
        return false;
    }
    if (_headerFields["host"].find(",") != std::string::npos ||
         _headerFields["host"].find(" ") != std::string::npos)
    {
        return false;
    }
    //TODO: Validate server_name max size
    return true;
}


void Request::validateHeader(void)
{
	if (validateContentLength() == false)
    {
          _statusCode = BAD_REQUEST;
          return;
    }
    if (validateHost() == false)
    {
        _statusCode = BAD_REQUEST;
        return;
    }
    if (findExtraRN() == true)
    {
        _statusCode = BAD_REQUEST;
        return;
    }
}
