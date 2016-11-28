#pragma once

#include <string>
#include <textfield.h>

#define TEMPORARY_FILE "C:\\Users\\wieis\\AppData\\Local\\Temp\\PANTHER_TEMP_TEST"

typedef std::string(*TestFunction)();

typedef std::string(*TextFieldTestFunction)(TextField* textField);

class Tester {
public:
	void RunTest(TestFunction testFunction);
	void RunTextFieldTest(std::string name, TextFieldTestFunction testFunction, std::string input, std::string expectedOutput);
};