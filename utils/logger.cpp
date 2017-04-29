
#include "logger.h"

static Logger* logger = nullptr;

Logger::Logger() {
	PGFileError error;
	file = panther::OpenFile(std::string(LOG_FILE), PGFileReadWrite, error);
}

Logger::~Logger() {
	panther::CloseFile(file);
}

void Logger::_WriteLogMessage(std::string message) {
	message = message + std::string("\n");
	panther::WriteToFile(file, message.c_str(), message.size());
	panther::Flush(file);
}

