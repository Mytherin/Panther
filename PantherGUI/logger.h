#pragma once

#include "mmap.h"

#define LOG_FILE "C:\\Users\\wieis\\Desktop\\log.txt"

class Logger {
public:
	static Logger* GetInstance() {
		static Logger logger;
		return &logger;
	}

	void WriteLogMessage(std::string message);

private:
	Logger();
	virtual ~Logger();

	PGFileHandle file;
};
