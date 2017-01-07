#pragma once

#include <string>
#include <textfield.h>

#define TEMPORARY_FILE "C:\\Users\\wieis\\AppData\\Local\\Temp\\PANTHER_TEMP_TEST"

typedef std::string(*TestFunction)();

typedef std::string(*TextFileTestFunction)(TextFile* textFile);

class Tester {
public:
	void RunTest(TestFunction testFunction);
	void RunTextFileTest(std::string name, TextFileTestFunction testFunction, std::string input, std::string expectedOutput);
	void RunTextFileFileTest(std::string name, TextFileTestFunction testFunction, std::string file_input, std::string expectedOutput);
};