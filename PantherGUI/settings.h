#pragma once

#include "utils.h"
#include <map>

class PGSettings {
public:
	void LoadSettings(std::string filename);

	bool GetSetting(std::string name, std::string& value);
private:
	std::map<std::string, std::string> settings;
};

class PGSettingsManager {
public:
	static PGSettingsManager* GetInstance() {
		static PGSettingsManager settingsmanager;
		return &settingsmanager;
	}

	static void Initialize() { (void) GetInstance(); }
	
	static bool GetSetting(std::string name, bool& value, PGSettings* extra_setting = nullptr);
	static bool GetSetting(std::string name, int& value, PGSettings* extra_setting = nullptr);
	static bool GetSetting(std::string name, double& value, PGSettings* extra_setting = nullptr);
	static bool GetSetting(std::string name, std::string& value, PGSettings* extra_setting = nullptr) { return GetInstance()->_GetSetting(name, value, extra_setting); }
private:
	bool _GetSetting(std::string name, std::string& value, PGSettings* extra_setting = nullptr);

	PGSettingsManager();

	PGSettings default_settings;
	PGSettings user_settings;

	std::map<std::string, PGSettings> styles;
};

