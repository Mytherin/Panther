
#include "textfield.h"
#include "tester.h"
#include <iostream>

std::string SimpleDeletion(TextField* textField);
std::string ForwardDeletion(TextField* textField);
std::string ForwardWordDeletion(TextField* textField);
std::string SelectionDeletion(TextField* textField);
std::string DeleteNewline(TextField* textField);
std::string ForwardDeleteNewline(TextField* textField);
std::string DetectUnixNewlineType(TextField* textField);
std::string DetectWindowsNewlineType(TextField* textField);
std::string DetectMacOSNewlineType(TextField* textField);

int main() {
	Tester tester;

	tester.RunTextFieldTest("Simple Deletion", SimpleDeletion, "hello world", "hllo world");
	tester.RunTextFieldTest("Forward Deletion", ForwardDeletion, "hello world", "ello world");
	tester.RunTextFieldTest("Forward Word Deletion", ForwardWordDeletion, "hello world", " world");
	tester.RunTextFieldTest("Selection Deletion", SelectionDeletion, "hello world", "ho world");
	tester.RunTextFieldTest("Delete Newline", DeleteNewline, "hello\n world", "hello world");
	tester.RunTextFieldTest("Forward Delete Newline", ForwardDeleteNewline, "hello\n world", "hello world");
	tester.RunTextFieldTest("Detect Newline Type (Unix)", DetectUnixNewlineType, "hello\nworld", "hello\nworld");
	tester.RunTextFieldTest("Detect Newline Type (Windows)", DetectWindowsNewlineType, "hello\r\nworld", "hello\nworld");
	tester.RunTextFieldTest("Detect Newline Type (MacOS)", DetectMacOSNewlineType, "hello\rworld", "hello\nworld");


	std::string line;
	std::getline(std::cin, line);
}

std::string SimpleDeletion(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonBackspace, PGModifierNone);
	return std::string("");
}

std::string ForwardDeletion(TextField* textField) {
	textField->KeyboardButton(PGButtonDelete, PGModifierNone);
	return std::string("");
}

std::string ForwardWordDeletion(TextField* textField) {
	textField->KeyboardButton(PGButtonDelete, PGModifierCtrl);
	return std::string("");
}

std::string SelectionDeletion(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonBackspace, PGModifierNone);
	return std::string("");
}

std::string DeleteNewline(TextField* textField) {
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonBackspace, PGModifierNone);
	return std::string("");
}

std::string ForwardDeleteNewline(TextField* textField) {
	textField->KeyboardButton(PGButtonEnd, PGModifierNone);
	textField->KeyboardButton(PGButtonDelete, PGModifierNone);
	return std::string("");
}

std::string DetectUnixNewlineType(TextField* textField) {
	if (textField->GetTextFile().GetLineEnding() != PGLineEndingUnix) {
		return std::string("Failed to detect correct line ending type.");
	}
	return std::string("");
}

std::string DetectWindowsNewlineType(TextField* textField) {
	if (textField->GetTextFile().GetLineEnding() != PGLineEndingWindows) {
		return std::string("Failed to detect correct line ending type.");
	}
	return std::string("");
}

std::string DetectMacOSNewlineType(TextField* textField) {
	if (textField->GetTextFile().GetLineEnding() != PGLineEndingMacOS) {
		return std::string("Failed to detect correct line ending type.");
	}
	return std::string("");
}
