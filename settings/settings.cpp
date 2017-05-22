
#include "mmap.h"
#include "settings.h"
#include "json.h"
#include "utils.h"

#include <algorithm>
#include <string>

#include "default-settings.h"

using namespace nlohmann;

void PGSettings::LoadSettings(std::string filename) {
	lng result_size;
	PGFileError error;
	char* ptr = (char*) panther::ReadFile(filename, result_size, error);
	if (!ptr) {
		return;
	}
	LoadSettingsFromData(ptr);
	panther::DestroyFileContents(ptr);
}

void PGSettings::LoadSettingsFromData(const char* data) {
	json j;
	try {
		j = json::parse(data);
	} catch (...) {
		return;
	}
	j.flatten();

	for (json::iterator it = j.begin(); it != j.end(); ++it) {
		settings[StripQuotes(it.key())] = StripQuotes(it.value().dump());
	}
}

bool PGSettings::GetSetting(std::string name, std::string& value) {
	if (settings.count(name) == 0) {
		return false;
	}
	value = settings[name];
	return true;
}

bool PGSettings::GetSetting(std::string name, std::vector<std::string>& value) {
	if (multiple_settings.count(name) > 0) {
		value = multiple_settings[name];
		return true;
	}
	return false;
}

void PGSettings::SetSetting(std::string name, std::string setting) {
	settings[name] = setting;
}

void PGSettings::SetSetting(std::string name, std::vector<std::string> setting) {
	multiple_settings[name] = setting;
}

void PGSettings::SetSetting(std::string name, bool value) {
	SetSetting(name, value ? std::string("true") : std::string("false"));
}

void PGSettings::SetSetting(std::string name, int value) {
	SetSetting(name, std::to_string(value));
}

void PGSettings::SetSetting(std::string name, double value) {
	SetSetting(name, std::to_string(value));
}

void PGSettings::SetSetting(std::string name, const char* setting) {
	SetSetting(name, std::string(setting));
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

bool PGSettingsManager::GetSetting(std::string name, lng& value, PGSettings* extra_setting) {
	std::string val;
	if (!GetSetting(name, val, extra_setting)) {
		return false;
	}
	errno = 0;
	lng intval = std::strtol(val.c_str(), nullptr, 10);
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
	default_settings.LoadSettingsFromData(PANTHER_DEFAULT_SETTINGS);
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

