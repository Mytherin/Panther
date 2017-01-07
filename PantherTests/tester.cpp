

#include "tester.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

void Tester::RunTest(TestFunction testFunction) {
	std::string result = testFunction();
	if (result.size() != 0) {
		std::cout << "Tester failed test." << std::endl;
		std::cout << result << std::endl;
	}
}

void Tester::RunTextFileTest(std::string name, TextFileTestFunction testFunction, std::string input, std::string expectedOutput) {
	// write the input to the file
	FILE* f;
#ifdef WIN32
	fopen_s(&f, TEMPORARY_FILE, "wb+");
#else
	f = fopen(TEMPORARY_FILE, "wb+");
#endif
	fwrite(input.c_str(), input.size(), 1, f);
	fclose(f);
	// create a temporary textfile
	TextFile* textfile = new TextFile(nullptr, TEMPORARY_FILE, true);
	// run the actual test
	std::string result = testFunction(textfile);
	if (result.size() != 0) {
		std::cout << "Tester failed test " << name << " with output:" << std::endl;
		std::cout << result << std::endl;
		delete textfile;
		return;
	}
	// if the test did not return an error message, check the output of the text field after the operation
	std::string resultingText = "";
	for (int i = 0; i < textfile->GetLineCount(); i++) {
		resultingText += std::string(textfile->GetLine(i).GetLine(), textfile->GetLine(i).GetLength());
		if (i + 1 < textfile->GetLineCount())
			resultingText += "\n";
	}
	if (resultingText != expectedOutput) {
		std::cout << "Tester failed test " << name << " with incorrect output." << std::endl;
		std::cout << "Expected output:" << std::endl;
		std::cout << expectedOutput << std::endl;
		std::cout << "Actual output:" << std::endl;
		std::cout << resultingText << std::endl;
	} else {
		std::cout << "SUCCESS: Test " << name << std::endl;
	}
}

void Tester::RunTextFileFileTest(std::string name, TextFileTestFunction testFunction, std::string path, std::string expectedOutput) {
	std::ostringstream buf;
	path = std::string(__FILE__).substr(0, strlen(__FILE__) - strlen("tester.cpp")) + path;
	std::ifstream input (path.c_str()); 
	buf << input.rdbuf(); 
	RunTextFileTest(name, testFunction, buf.str(), expectedOutput);
}
