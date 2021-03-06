
#include "language.h"

PGLanguageManager::PGLanguageManager() {

}

PGLanguageManager::~PGLanguageManager() {

}

void PGLanguageManager::_AddLanguage(PGLanguage* language) {
	languages.push_back(language);
}

PGLanguage* PGLanguageManager::_GetLanguage(std::string extension) {
	for (auto it = languages.begin(); it != languages.end(); it++) {
		if ((*it)->MatchesFileExtension(extension)) {
			return *it;
		}
	}
	return nullptr;
}

PGLanguage* PGLanguageManager::_GetLanguageFromName(std::string name) {
	for (auto it = languages.begin(); it != languages.end(); it++) {
		if ((*it)->GetName() == name) {
			return *it;
		}
	}
	return nullptr;
}