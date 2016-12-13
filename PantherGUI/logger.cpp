
#include "logger.h"

static Logger* logger = nullptr;

Logger::Logger() {
	file = PGmmap::OpenFile(std::string(LOG_FILE), PGFileReadWrite);
}

Logger::~Logger() {
	PGmmap::CloseFile(file);
}

void Logger::_WriteLogMessage(std::string message) {
	message = message + std::string("\n");
	PGmmap::WriteToFile(file, message.c_str(), message.size());
	PGmmap::Flush(file);
}

