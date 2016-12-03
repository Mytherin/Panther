
#include "textfield.h"
#include "tester.h"
#include <iostream>

std::string SimpleDeletion(TextField* textField);
std::string ForwardDeletion(TextField* textField);
std::string ForwardWordDeletion(TextField* textField);
std::string SelectionDeletion(TextField* textField);
std::string DeleteNewline(TextField* textField);
std::string ForwardDeleteNewline(TextField* textField);
std::string SimpleInsertText(TextField* textField);
std::string SimpleInsertNewline(TextField* textField);
std::string SimpleCopyPaste(TextField* textField);
std::string MultilineCopyPaste(TextField* textField);
std::string MultilineCopyPasteReplaceText(TextField* textField);
std::string ManyLinesCopyPaste(TextField* textField);
std::string UndoSimpleDeletion(TextField* textField);
std::string UndoForwardDeletion(TextField* textField);
std::string UndoForwardWordDeletion(TextField* textField);
std::string UndoSelectionDeletion(TextField* textField);
std::string UndoDeleteNewline(TextField* textField);
std::string UndoForwardDeleteNewline(TextField* textField);
std::string UndoSimpleInsertText(TextField* textField);
std::string UndoSimpleInsertNewline(TextField* textField);
std::string UndoSimpleCopyPaste(TextField* textField);
std::string UndoMultilineCopyPaste(TextField* textField);
std::string UndoMultilineCopyPasteReplaceText(TextField* textField);
std::string UndoManyOperations(TextField* textField);
std::string RedoManyOperations(TextField* textField);
std::string MixedUndoRedo(TextField* textField);
std::string MultiCursorInsert(TextField* textField);
std::string MultiCursorNewline(TextField* textField);
std::string MultiCursorDeletion(TextField* textField);
std::string MultiCursorSelectionDeletion(TextField* textField);
std::string MultiCursorWordDeletion(TextField* textField);
std::string MultiCursorOverlappingCursors(TextField* textField);
std::string MultiCursorOverlappingSelection(TextField* textField);
std::string MultiCursorUndoInsert(TextField* textField);
std::string MultiCursorUndoDelete(TextField* textField);
std::string MultiCursorUndoComplex(TextField* textField);
std::string MultiCursorRedoComplex(TextField* textField);
std::string PasteInsertUndoPaste(TextField* textField);
std::string MultiLineDelete(TextField* textField);
std::string MultiLineMultiCursorDelete(TextField* textField);
std::string MultiCursorDeleteInsert(TextField* textField);
std::string PartialDeleteCursorSameLine(TextField* textField);
std::string MultiCursorPasteSameLine(TextField* textField);
std::string TestingTestCase(TextField* textField);
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
	tester.RunTextFieldTest("Simple Insert Text", SimpleInsertText, "hello world", "helloa world");
	tester.RunTextFieldTest("Simple Insert Newline", SimpleInsertNewline, "hello world", "hello\n\n world");
	tester.RunTextFieldTest("Simple Copy Paste", SimpleCopyPaste, "hello world", "hellohello world");
	tester.RunTextFieldTest("Multiline Copy Paste", MultilineCopyPaste, "hello world\nhow are you doing?", "hello world\nhow are you doing?hello world\nhow are you doing?");
	tester.RunTextFieldTest("Multiline Copy Paste In Text", MultilineCopyPasteReplaceText, "hello world\nhow are you doing?", "hello world\nhello world\nhow are you doing? are you doing?");
	tester.RunTextFieldTest("Many Lines Copy Paste", ManyLinesCopyPaste, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\n\n\n\n\nhello\n", "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\n\n\n\n\nhello\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\n\n\n\n\nhello\n");

	tester.RunTextFieldTest("Undo Simple Deletion", UndoSimpleDeletion, "hello world", "hello world");
	tester.RunTextFieldTest("Undo Forward Deletion", UndoForwardDeletion, "hello world", "hello world");
	tester.RunTextFieldTest("Undo Forward Word Deletion", UndoForwardWordDeletion, "hello world", "hello world");
	tester.RunTextFieldTest("Undo Selection Deletion", UndoSelectionDeletion, "hello world", "hello world");
	tester.RunTextFieldTest("Undo Delete Newline", UndoDeleteNewline, "hello\n world", "hello\n world");
	tester.RunTextFieldTest("Undo Forward Delete Newline", UndoForwardDeleteNewline, "hello\n world", "hello\n world");
	tester.RunTextFieldTest("Undo Simple Insert Text", UndoSimpleInsertText, "hello world", "hello world");
	tester.RunTextFieldTest("Undo Simple Insert Newline", UndoSimpleInsertNewline, "hello world", "hello world");
	tester.RunTextFieldTest("Undo Simple Copy Paste", UndoSimpleCopyPaste, "hello world", "hello world");
	tester.RunTextFieldTest("Undo Multiline Copy Paste", UndoMultilineCopyPaste, "hello world\nhow are you doing?", "hello world\nhow are you doing?");
	tester.RunTextFieldTest("Undo Multiline Copy Paste In Text", UndoMultilineCopyPasteReplaceText, "hello world\nhow are you doing?", "hello world\nhow are you doing?");
	
	tester.RunTextFieldTest("Undo Many Operations", UndoManyOperations, "hello world\nhow are you doing?", "ahello world\nhow are you doing?");
	tester.RunTextFieldTest("Redo Many Operations", RedoManyOperations, "hello world\nhow are you doing?", "\nhell\nhello worldhello world\nhow are you doing?hello worldo");
	tester.RunTextFieldTest("Mixed Undo Redo", MixedUndoRedo, "hello world\nhow are you doing?", "\nhell\nhello worldhello world\nhow are you doing?hello worldo");
	
	tester.RunTextFieldTest("Multi Cursor Insert", MultiCursorInsert, "hello world", "haello waorld");
	tester.RunTextFieldTest("Multi Cursor Newline", MultiCursorNewline, "hello world", "h\nello w\norld");
	tester.RunTextFieldTest("Multi Cursor Deletion", MultiCursorDeletion, "hello world", "hllo wrld");
	tester.RunTextFieldTest("Multi Cursor Selection Deletion", MultiCursorSelectionDeletion, "hello world", "hlo wld");
	tester.RunTextFieldTest("Multi Cursor Word Deletion", MultiCursorWordDeletion, "hello world", "a a");
	tester.RunTextFieldTest("Multi Cursor Overlapping Cursors", MultiCursorOverlappingCursors, "hello world", "ahello world");
	tester.RunTextFieldTest("Multi Cursor Overlapping Selection", MultiCursorOverlappingSelection, "hello world", "h");
	tester.RunTextFieldTest("Multi Cursor Undo Insert", MultiCursorUndoInsert, "hello world", "hbello wborld");
	tester.RunTextFieldTest("Multi Cursor Undo Deletion", MultiCursorUndoDelete, "hello world", "halo wald");
	tester.RunTextFieldTest("Multi Cursor Undo Complex", MultiCursorUndoComplex, "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n");
	tester.RunTextFieldTest("Multi Cursor Redo Complex", MultiCursorRedoComplex, "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n\n\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\ndef hello():\n\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n    return \"hello world\";\n\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n\n\nprint(hello())\n");
	
	tester.RunTextFieldTest("Paste Insert Undo Paste", PasteInsertUndoPaste, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n", "a");
	tester.RunTextFieldTest("Multi Line Delete", MultiLineDelete, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\naaorld\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n");
	tester.RunTextFieldTest("Multi Line Multi Cursor Delete", MultiLineMultiCursorDelete, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\naaorld\";\n\n\nprint(hello())\naaorld\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n");
	tester.RunTextFieldTest("Multi Cursor Delete + Insert", MultiCursorDeleteInsert, "\ndef hello():\n    print(\"hello world\")\n", "\ndefaa\"hellaao world\")\n");
	tester.RunTextFieldTest("Partial Multi Cursor Delete On Same Line", PartialDeleteCursorSameLine, "\n\n\nprint(\"hello world\")", "arint(\"ahello world\")");
	tester.RunTextFieldTest("Multi Cursor Paste Same Line", MultiCursorPasteSameLine, "\ndef hello():\n", "\n\ndef hello():\ndef hel\ndef hello():\nlo():\n");
	tester.RunTextFieldTest("TestingTestCase", TestingTestCase, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n", "a");

	std::string line;
	std::getline(std::cin, line);
}

static void TypeWord(char* string, TextField* textField) {
	while (*string) {
		textField->KeyboardCharacter(*string, PGModifierNone);
		string++;
	}
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

std::string SimpleInsertText(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardCharacter('a', PGModifierNone);
	return std::string("");
}

std::string SimpleInsertNewline(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	return std::string("");
}

std::string SimpleCopyPaste(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	return std::string("");
}

std::string MultilineCopyPaste(TextField* textField) {
	textField->KeyboardButton(PGButtonDown, PGModifierShift);
	textField->KeyboardButton(PGButtonEnd, PGModifierShift);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	return std::string("");
}

std::string MultilineCopyPasteReplaceText(TextField* textField) {
	textField->KeyboardButton(PGButtonDown, PGModifierShift);
	textField->KeyboardButton(PGButtonEnd, PGModifierShift);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonLeft, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	return std::string("");
}

std::string ManyLinesCopyPaste(TextField* textField) {
	textField->KeyboardButton(PGButtonEnd, PGModifierCtrlShift);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	return std::string("");
}


std::string UndoSimpleDeletion(TextField* textField) {
	SimpleDeletion(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoForwardDeletion(TextField* textField) {
	ForwardDeletion(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoForwardWordDeletion(TextField* textField) {
	ForwardWordDeletion(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoSelectionDeletion(TextField* textField) {
	SelectionDeletion(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoDeleteNewline(TextField* textField) {
	DeleteNewline(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoForwardDeleteNewline(TextField* textField) {
	ForwardDeleteNewline(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoSimpleInsertText(TextField* textField) {
	SimpleInsertText(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoSimpleInsertNewline(TextField* textField) {
	SimpleInsertNewline(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoSimpleCopyPaste(TextField* textField) {
	SimpleCopyPaste(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoMultilineCopyPaste(TextField* textField) {
	MultilineCopyPaste(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoMultilineCopyPasteReplaceText(TextField* textField) {
	MultilineCopyPasteReplaceText(textField);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	return std::string("");
}

std::string UndoManyOperations(TextField* textField) {
	int i = 0;
	textField->KeyboardButton(PGButtonEnd, PGModifierShift);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	TypeWord("hello", textField);
	textField->KeyboardButton(PGButtonLeft, PGModifierShift);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardCharacter('o', PGModifierNone);

	for (i = 0; i < 12; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}

	textField->KeyboardButton(PGButtonLeft, PGModifierNone);
	textField->KeyboardCharacter('a', PGModifierNone);
	return std::string("");
}

std::string RedoManyOperations(TextField* textField) {
	int i = 0;
	textField->KeyboardButton(PGButtonEnd, PGModifierShift);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	TypeWord("hello", textField);
	textField->KeyboardButton(PGButtonLeft, PGModifierShift);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardCharacter('o', PGModifierNone);

	for (i = 0; i < 6; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}
	for (i = 0; i < 6; i++) {
		textField->KeyboardCharacter('Y', PGModifierCtrl);
	}
	for (i = 0; i < 12; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}
	for (i = 0; i < 12; i++) {
		textField->KeyboardCharacter('Y', PGModifierCtrl);
	}

	return std::string("");
}

std::string MixedUndoRedo(TextField* textField) {
	int i = 0;
	textField->KeyboardButton(PGButtonEnd, PGModifierShift);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	TypeWord("hello", textField);
	textField->KeyboardButton(PGButtonLeft, PGModifierShift);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	textField->KeyboardButton(PGButtonUp, PGModifierNone);
	textField->KeyboardButton(PGButtonHome, PGModifierNone);
	textField->KeyboardCharacter('Y', PGModifierCtrl);
	textField->KeyboardCharacter('Y', PGModifierCtrl);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->KeyboardCharacter('V', PGModifierCtrl);
	textField->KeyboardCharacter('o', PGModifierNone);

	for (i = 0; i < 6; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}
	for (i = 0; i < 6; i++) {
		textField->KeyboardCharacter('Y', PGModifierCtrl);
	}
	for (i = 0; i < 12; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}
	for (i = 0; i < 12; i++) {
		textField->KeyboardCharacter('Y', PGModifierCtrl);
	}

	return std::string("");
}

std::string MultiCursorInsert(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardCharacter('a', PGModifierNone);

	return std::string("");
}

std::string MultiCursorNewline(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonEnter, PGModifierNone);

	return std::string("");
}

std::string MultiCursorDeletion(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonDelete, PGModifierNone);

	return std::string("");
}

std::string MultiCursorSelectionDeletion(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonRight, PGModifierShift);
	textField->KeyboardButton(PGButtonDelete, PGModifierNone);

	return std::string("");
}

std::string MultiCursorWordDeletion(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonBackspace, PGModifierCtrl);
	textField->KeyboardCharacter('a', PGModifierNone);

	return std::string("");
}

std::string MultiCursorOverlappingCursors(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonHome, PGModifierNone);
	textField->KeyboardCharacter('a', PGModifierNone);

	return std::string("");
}

std::string MultiCursorOverlappingSelection(TextField* textField) {
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->KeyboardButton(PGButtonRight, PGModifierCtrl);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonRight, PGModifierNone);
	textField->KeyboardButton(PGButtonEnd, PGModifierShift);
	textField->KeyboardButton(PGButtonBackspace, PGModifierNone);

	return std::string("");
}

std::string MultiCursorUndoInsert(TextField* textField) {
	MultiCursorInsert(textField);
	textField->KeyboardButton(PGButtonEnd, PGModifierNone);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	textField->KeyboardCharacter('b', PGModifierNone);
	return std::string("");
}

std::string MultiCursorUndoDelete(TextField* textField) {
	MultiCursorSelectionDeletion(textField);
	textField->KeyboardButton(PGButtonEnd, PGModifierNone);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	textField->KeyboardCharacter('a', PGModifierNone);
	return std::string("");
}

std::string MultiCursorUndoComplex(TextField* textField) {
	textField->KeyboardCharacter('A', PGModifierCtrl);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonLeft, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	TypeWord("hello", textField);
	textField->KeyboardButton(PGButtonLeft, PGModifierCtrlShift);
	textField->KeyboardCharacter('V', PGModifierCtrl);


	for (int i = 0; i < 6; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}

	return std::string("");
}

std::string MultiCursorRedoComplex(TextField* textField) {
	textField->KeyboardCharacter('A', PGModifierCtrl);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	textField->KeyboardButton(PGButtonLeft, PGModifierNone);
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	textField->KeyboardButton(PGButtonDown, PGModifierNone);
	textField->GetCursors().push_back(new Cursor(&textField->GetTextFile(), 0, 0));
	TypeWord("hello", textField);
	textField->KeyboardButton(PGButtonLeft, PGModifierCtrlShift);
	textField->KeyboardCharacter('V', PGModifierCtrl);


	for (int i = 0; i < 2; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}
	for (int i = 0; i < 2; i++) {
		textField->KeyboardCharacter('Y', PGModifierCtrl);
	}
	for (int i = 0; i < 6; i++) {
		textField->KeyboardCharacter('Z', PGModifierCtrl);
	}
	for (int i = 0; i < 6; i++) {
		textField->KeyboardCharacter('Y', PGModifierCtrl);
	}

	return std::string("");
}

std::string PasteInsertUndoPaste(TextField* textField) {
	textField->KeyboardCharacter('A', PGModifierCtrl);
	textField->KeyboardCharacter('a', PGModifierNone);
	textField->KeyboardCharacter('Z', PGModifierCtrl);
	textField->KeyboardCharacter('a', PGModifierNone);
	return std::string("");
}

std::string MultiLineDelete(TextField* textField) {
	std::vector<Cursor*>& cursors = textField->GetCursors();
	cursors[0]->SetCursorLocation(14, 16);
	cursors[0]->SetCursorStartLocation(12, 0);
	textField->KeyboardCharacter('a', PGModifierNone);
	textField->KeyboardCharacter('a', PGModifierNone);

	return std::string("");
}

std::string MultiLineMultiCursorDelete(TextField* textField) {
	std::vector<Cursor*>& cursors = textField->GetCursors();
	cursors[0]->SetCursorLocation(14, 16);
	cursors[0]->SetCursorStartLocation(12, 0);
	cursors.push_back(new Cursor(&textField->GetTextFile(), 20, 16));
	cursors[1]->SetCursorStartLocation(18, 0);
	textField->KeyboardCharacter('a', PGModifierNone);
	textField->KeyboardCharacter('a', PGModifierNone);

	return std::string("");
}

std::string MultiCursorDeleteInsert(TextField* textField) {
	std::vector<Cursor*>& cursors = textField->GetCursors();
	Cursor*& active_cursor = textField->GetActiveCursor();
	TextFile& textfile = textField->GetTextFile();

	cursors[0]->SetCursorLocation(1, 3);
	cursors[0]->SetCursorStartLocation(2, 10);
	cursors.push_back(new Cursor(&textfile, 2, 15));
	textField->KeyboardCharacter('a', PGModifierNone);
	textField->KeyboardCharacter('a', PGModifierNone);


	return std::string("");
}

std::string PartialDeleteCursorSameLine(TextField* textField) {
	std::vector<Cursor*>& cursors = textField->GetCursors();
	Cursor*& active_cursor = textField->GetActiveCursor();
	TextFile& textfile = textField->GetTextFile();

	cursors[0]->SetCursorStartLocation(3, 1);
	cursors.push_back(new Cursor(&textfile, 3, 7));
	textField->KeyboardCharacter('a', PGModifierNone);


	return std::string("");
}

std::string MultiCursorPasteSameLine(TextField* textField) {
	std::vector<Cursor*>& cursors = textField->GetCursors();
	Cursor*& active_cursor = textField->GetActiveCursor();
	TextFile& textfile = textField->GetTextFile();

	textField->KeyboardCharacter('A', PGModifierCtrl);
	textField->KeyboardCharacter('C', PGModifierCtrl);
	cursors[0]->SetCursorLocation(1, 0);
	cursors.push_back(new Cursor(&textfile, 1, 7));
	textField->KeyboardCharacter('V', PGModifierCtrl);

	return std::string("");
}

std::string TestingTestCase(TextField* textfield) {
	TextField* textField = textfield;
	std::vector<Cursor*>& cursors = textfield->GetCursors();
	Cursor*& active_cursor = textfield->GetActiveCursor();
	TextFile& textfile = textfield->GetTextFile();

	active_cursor = new Cursor(&textfile, 5, 0);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 9);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(69, PGModifierCtrl);
	textField->KeyboardCharacter(87, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(70, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(5, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(87, PGModifierCtrl);
	textField->KeyboardCharacter(69, PGModifierCtrl);
	active_cursor = new Cursor(&textfile, 5, 0);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 13);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(82, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 9);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(87, PGModifierCtrl);
	textField->KeyboardCharacter(69, PGModifierCtrl);
	active_cursor = new Cursor(&textfile, 8, 12);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(82, PGModifierCtrl);
	textField->KeyboardCharacter(87, PGModifierCtrl);
	textField->KeyboardCharacter(69, PGModifierCtrl);
	active_cursor = new Cursor(&textfile, 9, 12);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(10, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(119, PGModifierNone);
	active_cursor = new Cursor(&textfile, 8, 0);
	cursors.push_back(active_cursor);
	textField->KeyboardCharacter(82, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(119, PGModifierNone);
	textField->KeyboardCharacter(82, PGModifierCtrl);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(10, 12);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(10, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(119, PGModifierNone);
	textField->KeyboardCharacter(101, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 8);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 9);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(114, PGModifierNone);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(7, 10);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 9);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(114, PGModifierNone);
	active_cursor = new Cursor(&textfile, 7, 9);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(87, PGModifierCtrl);
	textField->KeyboardCharacter(69, PGModifierCtrl);
	textField->KeyboardCharacter(82, PGModifierCtrl);
	textField->KeyboardCharacter(119, PGModifierNone);
	active_cursor = new Cursor(&textfile, 9, 11);
	cursors.push_back(active_cursor);
	textField->KeyboardCharacter(82, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	active_cursor = new Cursor(&textfile, 7, 0);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(114, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(10, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(119, PGModifierNone);
	textField->KeyboardCharacter(101, PGModifierNone);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(8, 3);
	textField->KeyboardCharacter(114, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(87, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 13);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(69, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(5, 2);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(114, PGModifierNone);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(6, 13);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(119, PGModifierNone);
	textField->KeyboardCharacter(101, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(7, 0);
	textField->KeyboardCharacter(119, PGModifierNone);
	textField->KeyboardCharacter(101, PGModifierNone);
	textField->KeyboardCharacter(116, PGModifierNone);
	active_cursor = new Cursor(&textfile, 9, 5);
	cursors.push_back(active_cursor);
	textField->KeyboardCharacter(87, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 9);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 3);
	Cursor::NormalizeCursors(textfield, cursors, false);
	active_cursor = new Cursor(&textfile, 6, 8);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 3);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 3);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(8, 10);
	textField->KeyboardCharacter(115, PGModifierNone);
	textField->KeyboardCharacter(100, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 3);
	Cursor::NormalizeCursors(textfield, cursors, false);
	active_cursor = new Cursor(&textfile, 8, 13);
	cursors.push_back(active_cursor);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(70, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 8);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 7);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(115, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(9, 7);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(102, PGModifierNone);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(6, 9);
	textField->KeyboardCharacter(83, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 3);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 13);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 12);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(102, PGModifierNone);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(7, 0);
	textField->KeyboardCharacter(115, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 5);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(68, PGModifierCtrl);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 6);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(102, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(5, 3);
	Cursor::NormalizeCursors(textfield, cursors, false);
	active_cursor = new Cursor(&textfile, 7, 0);
	cursors.push_back(active_cursor);
	textField->KeyboardCharacter(115, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(83, PGModifierCtrl);
	active_cursor = new Cursor(&textfile, 5, 4);
	cursors.push_back(active_cursor);
	textField->KeyboardCharacter(71, PGModifierCtrl);
	textField->KeyboardCharacter(100, PGModifierNone);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(6, 6);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 11);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 10);
	Cursor::NormalizeCursors(textfield, cursors, false);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(8, 9);
	Cursor::NormalizeCursors(textfield, cursors, false);
	textField->KeyboardCharacter(103, PGModifierNone);
	if (cursors.size() > 1) {
		cursors.erase(cursors.begin() + 1, cursors.end());
	}
	cursors[0]->SetCursorLocation(8, 9);
	if (!active_cursor) active_cursor = cursors.front();
	active_cursor->SetCursorStartLocation(7, 0);
	Cursor::NormalizeCursors(textfield, cursors, false);


	return std::string("");
}
