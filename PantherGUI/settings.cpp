
#include "mmap.h"
#include "settings.h"
#include "json.h"

#include <algorithm>
#include <string> 

using namespace nlohmann;

std::string StripQuotes(std::string str) {
	if (str[0] == '"')
		str = str.substr(1);
	if (str[str.size() - 1] == '"')
		str = str.substr(0, str.size() - 1);
	return str;
}

void PGSettings::LoadSettings(std::string filename) {
	lng result_size;
	char* ptr = (char*) panther::ReadFile(filename, result_size);
	if (!ptr) {
		assert(0);
		return;
	}
	json j;
	try {
		j = json::parse(ptr);
	} catch(...) {
		return;
	}
	j.flatten();

	for (json::iterator it = j.begin(); it != j.end(); ++it) {
		settings[StripQuotes(it.key())] = StripQuotes(it.value().dump());
	}

	panther::DestroyFileContents(ptr);
}

bool PGSettings::GetSetting(std::string name, std::string& value) {
	if (settings.count(name) == 0) {
		return false;
	}
	value = settings[name];
	return true;
}

bool PGSettingsManager::GetSetting(std::string name, bool& value, PGSettings* extra_setting) {
	std::string val;
	if (!GetSetting(name, val, extra_setting)) {
		return false;
	}
	std::transform(val.begin(), val.end(), val.begin(), ::tolower);
	value = val == "true";
	return true;
}

bool PGSettingsManager::GetSetting(std::string name, int& value, PGSettings* extra_setting) {
	std::string val;
	if (!GetSetting(name, val, extra_setting)) {
		return false;
	}
	errno = 0;
	long intval = std::strtol(val.c_str(), nullptr, 10);
	if (errno != 0) {
		errno = 0;
		return false;
	}
	value = intval;
	return true;
}

bool PGSettingsManager::GetSetting(std::string name, double& value, PGSettings* extra_setting) {
	assert(0);
	return false;
}

PGSettingsManager::PGSettingsManager() {
	default_settings.LoadSettings("default-settings.json");
}

bool PGSettingsManager::_GetSetting(std::string name, std::string& value, PGSettings* extra_setting) {
	if (extra_setting) {
		if (extra_setting->GetSetting(name, value)) {
			return true;
		}
	}
	if (user_settings.GetSetting(name, value)) {
		return true;
	}
	if (default_settings.GetSetting(name, value)) {
		return true;
	}
	return false;
}

