#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <iostream>

typedef enum
{
	DEBUG2,
	DEBUG,
	INFO, 
	ERROR,
} LogLevel;

class Logger
{
public:

	Logger(LogLevel level = DEBUG);

	void log(LogLevel level, const std::string& message);
	std::string currentTimeDate(void);
	std::string logLevelToString(LogLevel level) const;

private:
	LogLevel logLevel;
};

#endif //_LOGGER_HPP_
