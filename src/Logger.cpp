#include "Logger.hpp"

#include <ctime>

//INFO
#ifndef GREEN
#define GREEN "\033[32m"
#endif

//ERROR
#ifndef RED
#define RED "\033[31m"
#endif

//DEBUGS
#ifndef YELLOW
#define YELLOW "\033[33m"
#endif

Logger::Logger(LogLevel level): logLevel(level) 
{
}

std::string Logger::currentTimeDate(void)
{
	time_t now = time(NULL);

	char buf[80];
	strftime(buf, sizeof(buf), "%Y-%m-%d %X", localtime(&now));
	return buf;
}

std::string Logger::logLevelToString(LogLevel level) const
{
	switch(level)
	{
		case INFO: return "INFO";
		case DEBUG: return "DEBUG";
		case DEBUG2: return "DEBUG2";
		case ERROR: return "ERROR";
		default: return "UNKNOWN";
	
	}
}

void Logger::log(LogLevel level, const std::string& message)
{
	if (level >= logLevel)
	{
		if (level == DEBUG || level == DEBUG2)
		{
			std::cout << YELLOW << currentTimeDate() << " -  " << logLevelToString(level) << " - " << message << "\033[0m" << std::endl; 
		}
		else if (level == INFO)
		{
			std::cout << GREEN << currentTimeDate() << " -  " << logLevelToString(level) << " - " << message << "\033[0m" << std::endl; 
		}
		else if (level == ERROR)
		{
			std::cout << RED << currentTimeDate() << " -  " << logLevelToString(level) << " - " << message << "\033[0m" << std::endl; 
		}
		
	}
}
