#pragma once

#include "syntaxhighlighter.h"

class PGLanguage {
public:
	virtual std::string GetName() { return "Plain Text"; }
	virtual SyntaxHighlighter* CreateHighlighter() { return nullptr; }
	virtual bool MatchesFileExtension(std::string extension) { return false; }
};

class PGLanguageManager {
public:
	static PGLanguageManager* GetInstance() {
		static PGLanguageManager langman;
		return &langman;
	}

	static std::vector<PGLanguage*>& GetLanguages() { return PGLanguageManager::GetInstance()->_GetLanguages(); }
	static void AddLanguage(PGLanguage* language) { PGLanguageManager::GetInstance()->_AddLanguage(language); }
	static PGLanguage* GetLanguage(std::string extension) { return PGLanguageManager::GetInstance()->_GetLanguage(extension); }
private:
	PGLanguageManager();
	virtual ~PGLanguageManager();

	void _AddLanguage(PGLanguage* language);
	PGLanguage* _GetLanguage(std::string extension);
	std::vector<PGLanguage*>& _GetLanguages() { return languages; }

	std::vector<PGLanguage*> languages;
};