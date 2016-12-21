#pragma once

#include "mmap.h"

#ifdef WIN32
#define LOG_FILE "C:\\Users\\wieis\\Desktop\\log.txt"
#else
#define LOG_FILE "/Users/myth/log.txt"
#endif

class Logger {
public:
	static Logger* GetInstance() {
		static Logger logger;
		return &logger;
	}

	static void WriteLogMessage(std::string message) { Logger::GetInstance()->_WriteLogMessage(message); }
private:
	void _WriteLogMessage(std::string message);

	Logger();
	virtual ~Logger();

	PGFileHandle file;
};
