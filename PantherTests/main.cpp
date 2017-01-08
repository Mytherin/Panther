
#include "textfield.h"
#include "tester.h"
#include <iostream>

std::string SimpleDeletion(TextFile* textfile);
std::string ForwardDeletion(TextFile* textfile);
std::string ForwardWordDeletion(TextFile* textfile);
std::string SelectionDeletion(TextFile* textfile);
std::string DeleteNewline(TextFile* textfile);
std::string ForwardDeleteNewline(TextFile* textfile);
std::string SimpleInsertText(TextFile* textfile);
std::string SimpleInsertNewline(TextFile* textfile);
std::string SimpleCopyPaste(TextFile* textfile);
std::string MultilineCopyPaste(TextFile* textfile);
std::string MultilineCopyPasteReplaceText(TextFile* textfile);
std::string ManyLinesCopyPaste(TextFile* textfile);
std::string ManyLinesCopyPasteWhitespace(TextFile* textfile);
std::string UndoSimpleDeletion(TextFile* textfile);
std::string UndoForwardDeletion(TextFile* textfile);
std::string UndoForwardWordDeletion(TextFile* textfile);
std::string UndoSelectionDeletion(TextFile* textfile);
std::string UndoDeleteNewline(TextFile* textfile);
std::string UndoForwardDeleteNewline(TextFile* textfile);
std::string UndoSimpleInsertText(TextFile* textfile);
std::string UndoSimpleInsertNewline(TextFile* textfile);
std::string UndoSimpleCopyPaste(TextFile* textfile);
std::string UndoMultilineCopyPaste(TextFile* textfile);
std::string UndoMultilineCopyPasteReplaceText(TextFile* textfile);
std::string UndoManyOperations(TextFile* textfile);
std::string RedoManyOperations(TextFile* textfile);
std::string MixedUndoRedo(TextFile* textfile);
std::string MultiCursorInsert(TextFile* textfile);
std::string MultiCursorNewline(TextFile* textfile);
std::string MultiCursorDeletion(TextFile* textfile);
std::string MultiCursorSelectionDeletion(TextFile* textfile);
std::string MultiCursorWordDeletion(TextFile* textfile);
std::string MultiCursorOverlappingCursors(TextFile* textfile);
std::string MultiCursorOverlappingSelection(TextFile* textfile);
std::string MultiCursorUndoInsert(TextFile* textfile);
std::string MultiCursorUndoDelete(TextFile* textfile);
std::string MultiCursorUndoComplex(TextFile* textfile);
std::string MultiCursorRedoComplex(TextFile* textfile);
std::string PasteInsertUndoPaste(TextFile* textfile);
std::string MultiLineDelete(TextFile* textfile);
std::string MultiLineMultiCursorDelete(TextFile* textfile);
std::string MultiCursorDeleteInsert(TextFile* textfile);
std::string PartialDeleteCursorSameLine(TextFile* textfile);
std::string MultiCursorPasteSameLine(TextFile* textfile);
std::string UndoMultiNewLine(TextFile* textfile);
std::string DetectUnixNewlineType(TextFile* textfile);
std::string DetectWindowsNewlineType(TextFile* textfile);
std::string DetectMacOSNewlineType(TextFile* textfile);

std::string MultiCursorMultiLineDelete(TextFile* textfile);

std::string Testerino(TextFile* textfile);

int main() {
	Tester tester;
	
	Scheduler::Initialize();
	Scheduler::SetThreadCount(8);
	
	for (int i = 0; i < 100; i++) {
		tester.RunTextFileTest("Simple Insert Newline", SimpleInsertNewline, "hello world", "hello\n\n world");
	}
	tester.RunTextFileTest("Simple Deletion", SimpleDeletion, "hello world", "hllo world");
	tester.RunTextFileTest("Forward Deletion", ForwardDeletion, "hello world", "ello world");
	tester.RunTextFileTest("Forward Word Deletion", ForwardWordDeletion, "hello world", " world");
	tester.RunTextFileTest("Selection Deletion", SelectionDeletion, "hello world", "ho world");
	tester.RunTextFileTest("Delete Newline", DeleteNewline, "hello\n world", "hello world");
	tester.RunTextFileTest("Forward Delete Newline", ForwardDeleteNewline, "hello\n world", "hello world");
	tester.RunTextFileTest("Detect Newline Type (Unix)", DetectUnixNewlineType, "hello\nworld", "hello\nworld");
	tester.RunTextFileTest("Detect Newline Type (Windows)", DetectWindowsNewlineType, "hello\r\nworld", "hello\nworld");
	tester.RunTextFileTest("Detect Newline Type (MacOS)", DetectMacOSNewlineType, "hello\rworld", "hello\nworld");
	tester.RunTextFileTest("Simple Insert Text", SimpleInsertText, "hello world", "helloa world");
	tester.RunTextFileTest("Simple Insert Newline", SimpleInsertNewline, "hello world", "hello\n\n world");
	tester.RunTextFileTest("Simple Copy Paste", SimpleCopyPaste, "hello world", "hellohello world");
	tester.RunTextFileTest("Multiline Copy Paste", MultilineCopyPaste, "hello world\nhow are you doing?", "hello world\nhow are you doing?hello world\nhow are you doing?");
	tester.RunTextFileTest("Multiline Copy Paste In Text", MultilineCopyPasteReplaceText, "hello world\nhow are you doing?", "hello world\nhello world\nhow are you doing? are you doing?");
	tester.RunTextFileTest("Many Lines Copy Paste", ManyLinesCopyPaste, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\n\n\n\n\nhello\n", "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\n\n\n\n\nhello\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\n\n\n\n\nhello\n");
	tester.RunTextFileTest("Many Lines Copy Paste (Whitespace)", ManyLinesCopyPasteWhitespace, "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n");

	tester.RunTextFileTest("Undo Simple Deletion", UndoSimpleDeletion, "hello world", "hello world");
	tester.RunTextFileTest("Undo Forward Deletion", UndoForwardDeletion, "hello world", "hello world");
	tester.RunTextFileTest("Undo Forward Word Deletion", UndoForwardWordDeletion, "hello world", "hello world");
	tester.RunTextFileTest("Undo Selection Deletion", UndoSelectionDeletion, "hello world", "hello world");
	tester.RunTextFileTest("Undo Delete Newline", UndoDeleteNewline, "hello\n world", "hello\n world");
	tester.RunTextFileTest("Undo Forward Delete Newline", UndoForwardDeleteNewline, "hello\n world", "hello\n world");
	tester.RunTextFileTest("Undo Simple Insert Text", UndoSimpleInsertText, "hello world", "hello world");
	tester.RunTextFileTest("Undo Simple Insert Newline", UndoSimpleInsertNewline, "hello world", "hello world");
	tester.RunTextFileTest("Undo Simple Copy Paste", UndoSimpleCopyPaste, "hello world", "hello world");
	tester.RunTextFileTest("Undo Multiline Copy Paste", UndoMultilineCopyPaste, "hello world\nhow are you doing?", "hello world\nhow are you doing?");
	tester.RunTextFileTest("Undo Multiline Copy Paste In Text", UndoMultilineCopyPasteReplaceText, "hello world\nhow are you doing?", "hello world\nhow are you doing?");
	
	tester.RunTextFileTest("Undo Many Operations", UndoManyOperations, "hello world\nhow are you doing?", "ahello world\nhow are you doing?");
	tester.RunTextFileTest("Redo Many Operations", RedoManyOperations, "hello world\nhow are you doing?", "\nhell\nhello worldhello world\nhow are you doing?hello worldo");
	tester.RunTextFileTest("Mixed Undo Redo", MixedUndoRedo, "hello world\nhow are you doing?", "\nhell\nhello worldhello world\nhow are you doing?hello worldo");
	
	tester.RunTextFileTest("Multi Cursor Insert", MultiCursorInsert, "hello world", "haello waorld");
	tester.RunTextFileTest("Multi Cursor Newline", MultiCursorNewline, "hello world", "h\nello w\norld");
	tester.RunTextFileTest("Multi Cursor Deletion", MultiCursorDeletion, "hello world", "hllo wrld");
	tester.RunTextFileTest("Multi Cursor Selection Deletion", MultiCursorSelectionDeletion, "hello world", "hlo wld");
	tester.RunTextFileTest("Multi Cursor Word Deletion", MultiCursorWordDeletion, "hello world", "a a");
	tester.RunTextFileTest("Multi Cursor Overlapping Cursors", MultiCursorOverlappingCursors, "hello world", "ahello world");
	tester.RunTextFileTest("Multi Cursor Overlapping Selection", MultiCursorOverlappingSelection, "hello world", "h");
	tester.RunTextFileTest("Multi Cursor Undo Insert", MultiCursorUndoInsert, "hello world", "hbello wborld");
	tester.RunTextFileTest("Multi Cursor Undo Deletion", MultiCursorUndoDelete, "hello world", "halo wald");
	tester.RunTextFileTest("Multi Cursor Undo Complex", MultiCursorUndoComplex, "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n");
	tester.RunTextFileTest("Multi Cursor Redo Complex", MultiCursorRedoComplex, "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n\n\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\ndef hello():\n\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n    return \"hello world\";\n\ndef hello():\n    return \"hello world\";\n\n\nprint(hello())\n\n\nprint(hello())\n");
	
	tester.RunTextFileTest("Paste Insert Undo Paste", PasteInsertUndoPaste, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n", "a");
	tester.RunTextFileTest("Multi Line Delete", MultiLineDelete, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\naaorld\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n");
	tester.RunTextFileTest("Multi Line Multi Cursor Delete", MultiLineMultiCursorDelete, "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n", "\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\naaorld\";\n\n\nprint(hello())\naaorld\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n\ndef hello():\n	return \"hello world\";\n\n\nprint(hello())\n");
	tester.RunTextFileTest("Multi Cursor Delete + Insert", MultiCursorDeleteInsert, "\ndef hello():\n    print(\"hello world\")\n", "\ndefaa\"hellaao world\")\n");
	tester.RunTextFileTest("Partial Multi Cursor Delete On Same Line", PartialDeleteCursorSameLine, "\n\n\nprint(\"hello world\")", "arint(\"ahello world\")");
	tester.RunTextFileTest("Multi Cursor Paste Same Line", MultiCursorPasteSameLine, "\ndef hello():\n", "\n\ndef hello():\ndef hel\ndef hello():\nlo():\n");
	tester.RunTextFileTest("Undo Multi Newline", UndoMultiNewLine, "\ndef hello():\n	return \"hello world\";\n", "\ndef hello():\n\tretu\nrn \"h\nello world\";\n");

	tester.RunTextFileTest("Multi Cursor Multi Line Delete", MultiCursorMultiLineDelete, "hello world\nhow are you doing\n\nI am doing well", "hello world\nhow are you doing\n\nI am doing well");

	//tester.RunTextFileFileTest("Testerino", Testerino, "mserver.txt", "");

	std::cout << "Successfully completed all tests!" << std::endl;

	std::string line;
	std::getline(std::cin, line);
}

static void TypeWord(char* string, TextFile* textfile) {
	while (*string) {
		textfile->InsertText(*string);
		string++;
	}
}

std::string SimpleDeletion(TextFile* textfile) {
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->DeleteCharacter(PGDirectionLeft);
	return std::string("");
}

std::string ForwardDeletion(TextFile* textfile) {
	textfile->DeleteCharacter(PGDirectionRight);
	return std::string("");
}

std::string ForwardWordDeletion(TextFile* textfile) {
	textfile->DeleteWord(PGDirectionRight);
	return std::string("");
}

std::string SelectionDeletion(TextFile* textfile) {
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->DeleteCharacter(PGDirectionLeft);
	return std::string("");
}

std::string DeleteNewline(TextFile* textfile) {
	textfile->OffsetLine(1);
	textfile->DeleteCharacter(PGDirectionLeft);
	return std::string("");
}

std::string ForwardDeleteNewline(TextFile* textfile) {
	textfile->OffsetEndOfLine();
	textfile->DeleteCharacter(PGDirectionRight);
	return std::string("");
}

std::string DetectUnixNewlineType(TextFile* textfile) {
	if (textfile->GetLineEnding() != PGLineEndingUnix) {
		return std::string("Failed to detect correct line ending type.");
	}
	return std::string("");
}

std::string DetectWindowsNewlineType(TextFile* textfile) {
	if (textfile->GetLineEnding() != PGLineEndingWindows) {
		return std::string("Failed to detect correct line ending type.");
	}
	return std::string("");
}

std::string DetectMacOSNewlineType(TextFile* textfile) {
	if (textfile->GetLineEnding() != PGLineEndingMacOS) {
		return std::string("Failed to detect correct line ending type.");
	}
	return std::string("");
}

std::string SimpleInsertText(TextFile* textfile) {
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->InsertText('a');
	return std::string("");
}

std::string SimpleInsertNewline(TextFile* textfile) {
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->AddNewLine();
	textfile->AddNewLine();
	return std::string("");
}

std::string SimpleCopyPaste(TextFile* textfile) {
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->PasteText(GetClipboardText(nullptr));
	return std::string("");
}

std::string MultilineCopyPaste(TextFile* textfile) {
	textfile->OffsetSelectionLine(1);
	textfile->SelectEndOfLine();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->PasteText(GetClipboardText(nullptr));
	return std::string("");
}

std::string MultilineCopyPasteReplaceText(TextFile* textfile) {
	textfile->OffsetSelectionLine(1);
	textfile->SelectEndOfLine();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->OffsetCharacter(PGDirectionLeft);
	textfile->OffsetLine(1);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->PasteText(GetClipboardText(nullptr));
	return std::string("");
}

std::string ManyLinesCopyPaste(TextFile* textfile) {
	textfile->SelectEndOfFile();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->PasteText(GetClipboardText(nullptr));
	return std::string("");
}

std::string ManyLinesCopyPasteWhitespace(TextFile* textfile) {
	textfile->SelectEverything();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->PasteText(GetClipboardText(nullptr));
	return std::string("");
}


std::string UndoSimpleDeletion(TextFile* textfile) {
	SimpleDeletion(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoForwardDeletion(TextFile* textfile) {
	ForwardDeletion(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoForwardWordDeletion(TextFile* textfile) {
	ForwardWordDeletion(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoSelectionDeletion(TextFile* textfile) {
	SelectionDeletion(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoDeleteNewline(TextFile* textfile) {
	DeleteNewline(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoForwardDeleteNewline(TextFile* textfile) {
	ForwardDeleteNewline(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoSimpleInsertText(TextFile* textfile) {
	SimpleInsertText(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoSimpleInsertNewline(TextFile* textfile) {
	SimpleInsertNewline(textfile);
	textfile->Undo();
	textfile->Undo();
	return std::string("");
}

std::string UndoSimpleCopyPaste(TextFile* textfile) {
	SimpleCopyPaste(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoMultilineCopyPaste(TextFile* textfile) {
	MultilineCopyPaste(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoMultilineCopyPasteReplaceText(TextFile* textfile) {
	MultilineCopyPasteReplaceText(textfile);
	textfile->Undo();
	return std::string("");
}

std::string UndoManyOperations(TextFile* textfile) {
	int i = 0;
	textfile->SelectEndOfLine();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->AddNewLine();
	TypeWord("hello", textfile);
	textfile->OffsetSelectionCharacter(PGDirectionLeft);
	textfile->AddNewLine();
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->OffsetLine(1);
	textfile->OffsetLine(1);
	textfile->OffsetLine(1);
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->InsertText('o');

	for (i = 0; i < 12; i++) {
		textfile->Undo();
	}

	textfile->OffsetCharacter(PGDirectionLeft);
	textfile->InsertText('a');
	return std::string("");
}

std::string RedoManyOperations(TextFile* textfile) {
	int i = 0;
	textfile->SelectEndOfLine();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->AddNewLine();
	TypeWord("hello", textfile);
	textfile->OffsetSelectionCharacter(PGDirectionLeft);
	textfile->AddNewLine();
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->OffsetLine(1);
	textfile->OffsetLine(1);
	textfile->OffsetLine(1);
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->InsertText('o');

	for (i = 0; i < 6; i++) {
		textfile->Undo();
	}
	for (i = 0; i < 6; i++) {
		textfile->Redo();
	}
	for (i = 0; i < 12; i++) {
		textfile->Undo();
	}
	for (i = 0; i < 12; i++) {
		textfile->Redo();
	}

	return std::string("");
}

std::string MixedUndoRedo(TextFile* textfile) {
	int i = 0;
	textfile->SelectEndOfLine();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->AddNewLine();
	TypeWord("hello", textfile);
	textfile->OffsetSelectionCharacter(PGDirectionLeft);
	textfile->AddNewLine();
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->Undo();
	textfile->Undo();
	textfile->OffsetLine(-1);
	textfile->OffsetStartOfLine();
	textfile->Redo();
	textfile->Redo();
	textfile->OffsetLine(1);
	textfile->OffsetLine(1);
	textfile->OffsetLine(1);
	textfile->PasteText(GetClipboardText(nullptr));
	textfile->InsertText('o');

	for (i = 0; i < 6; i++) {
		textfile->Undo();
	}
	for (i = 0; i < 6; i++) {
		textfile->Redo();
	}
	for (i = 0; i < 12; i++) {
		textfile->Undo();
	}
	for (i = 0; i < 12; i++) {
		textfile->Redo();
	}

	return std::string("");
}

std::string MultiCursorInsert(TextFile* textfile) {
	textfile->OffsetWord(PGDirectionRight);
	textfile->OffsetWord(PGDirectionRight);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->InsertText('a');

	return std::string("");
}

std::string MultiCursorNewline(TextFile* textfile) {
	textfile->OffsetWord(PGDirectionRight);
	textfile->OffsetWord(PGDirectionRight);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->AddNewLine();

	return std::string("");
}

std::string MultiCursorDeletion(TextFile* textfile) {
	textfile->OffsetWord(PGDirectionRight);
	textfile->OffsetWord(PGDirectionRight);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->DeleteCharacter(PGDirectionRight);

	return std::string("");
}

std::string MultiCursorSelectionDeletion(TextFile* textfile) {
	textfile->OffsetWord(PGDirectionRight);
	textfile->OffsetWord(PGDirectionRight);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->OffsetSelectionCharacter(PGDirectionRight);
	textfile->DeleteCharacter(PGDirectionRight);

	return std::string("");
}

std::string MultiCursorWordDeletion(TextFile* textfile) {
	textfile->OffsetWord(PGDirectionRight);
	textfile->OffsetWord(PGDirectionRight);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetWord(PGDirectionRight);
	textfile->DeleteWord(PGDirectionLeft);
	textfile->InsertText('a');

	return std::string("");
}

std::string MultiCursorOverlappingCursors(TextFile* textfile) {
	textfile->OffsetWord(PGDirectionRight);
	textfile->OffsetWord(PGDirectionRight);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetStartOfLine();
	textfile->InsertText('a');

	return std::string("");
}

std::string MultiCursorOverlappingSelection(TextFile* textfile) {
	textfile->OffsetWord(PGDirectionRight);
	textfile->OffsetWord(PGDirectionRight);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetCharacter(PGDirectionRight);
	textfile->SelectEndOfLine();
	textfile->DeleteCharacter(PGDirectionLeft);

	return std::string("");
}

std::string MultiCursorUndoInsert(TextFile* textfile) {
	MultiCursorInsert(textfile);
	textfile->SelectEndOfLine();
	textfile->Undo();
	textfile->InsertText('b');
	return std::string("");
}

std::string MultiCursorUndoDelete(TextFile* textfile) {
	MultiCursorSelectionDeletion(textfile);
	textfile->OffsetEndOfLine();
	textfile->Undo();
	textfile->InsertText('a');
	return std::string("");
}

std::string MultiCursorUndoComplex(TextFile* textfile) {
	textfile->SelectEverything();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->OffsetCharacter(PGDirectionLeft);
	textfile->OffsetLine(1);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetLine(1);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetLine(1);
	textfile->AddNewCursor(0, 0);
	TypeWord("hello", textfile);
	textfile->OffsetSelectionWord(PGDirectionLeft);
	textfile->PasteText(GetClipboardText(nullptr));


	for (int i = 0; i < 6; i++) {
		textfile->Undo();
	}

	return std::string("");
}

std::string MultiCursorRedoComplex(TextFile* textfile) {
	textfile->SelectEverything();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->OffsetCharacter(PGDirectionLeft);
	textfile->OffsetLine(1);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetLine(1);
	textfile->AddNewCursor(0, 0);
	textfile->OffsetLine(1);
	textfile->AddNewCursor(0, 0);
	TypeWord("hello", textfile);
	textfile->OffsetSelectionWord(PGDirectionLeft);
	textfile->PasteText(GetClipboardText(nullptr));


	for (int i = 0; i < 2; i++) {
		textfile->Undo();
	}
	for (int i = 0; i < 2; i++) {
		textfile->Redo();
	}
	for (int i = 0; i < 6; i++) {
		textfile->Undo();
	}
	for (int i = 0; i < 6; i++) {
		textfile->Redo();
	}

	return std::string("");
}

std::string PasteInsertUndoPaste(TextFile* textfile) {
	textfile->SelectEverything();
	textfile->InsertText('a');
	textfile->Undo();
	textfile->InsertText('a');
	return std::string("");
}

std::string MultiLineDelete(TextFile* textfile) {
	textfile->SetCursorLocation(12, 0, 14, 16);
	textfile->InsertText('a');
	textfile->InsertText('a');

	return std::string("");
}

std::string MultiLineMultiCursorDelete(TextFile* textfile) {
	textfile->SetCursorLocation(12, 0, 14, 16);
	textfile->AddNewCursor(20, 16);
	textfile->GetCursors()[1]->SetCursorStartLocation(18, 0);
	textfile->InsertText('a');
	textfile->InsertText('a');

	return std::string("");
}

std::string MultiCursorDeleteInsert(TextFile* textfile) {
	textfile->SetCursorLocation(2, 10, 1, 3);
	textfile->AddNewCursor(2, 15);
	textfile->InsertText('a');
	textfile->InsertText('a');


	return std::string("");
}

std::string PartialDeleteCursorSameLine(TextFile* textfile) {
	textfile->SetCursorLocation(3, 1, 0, 0);
	textfile->AddNewCursor(3, 7);
	textfile->InsertText('a');

	return std::string("");
}

std::string MultiCursorPasteSameLine(TextFile* textfile) {
	textfile->SelectEverything();
	SetClipboardText(nullptr, textfile->CopyText());
	textfile->SetCursorLocation(1, 0);
	textfile->AddNewCursor(1, 7);
	textfile->PasteText(GetClipboardText(nullptr));

	return std::string("");
}

std::string UndoMultiNewLine(TextFile* textfield) {
	TextFile* textfile = textfield;
	textfile->SetCursorLocation(2, 5);
	textfile->AddNewCursor(2, 10);
	textfile->AddNewLine();
	textfile->Undo();
	textfile->AddNewLine();

	return std::string("");
}

std::string MultiCursorMultiLineDelete(TextFile* textfile) {
	textfile->SetCursorLocation(1, 5, 0, 1);
	textfile->AddNewCursor(3, 7);
	textfile->AddNewLine();
	textfile->Undo();
	textfile->AddNewLine();
	textfile->Undo();


	return std::string("");
}

std::string Testerino(TextFile* textfile) {
	return std::string("");
}
