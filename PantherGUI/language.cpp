
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