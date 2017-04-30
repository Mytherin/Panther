#pragma once

#include "utils.h"
#include <map>
#include <vector>

class PGSettings {
public:
	void LoadSettings(std::string filename);
	void LoadSettingsFromData(char* data);


	void SetSetting(std::string name, bool setting);
	void SetSetting(std::string name, int setting);
	void SetSetting(std::string name, double setting);
	void SetSetting(std::string name, const char* setting);
	void SetSetting(std::string name, std::string setting);
	void SetSetting(std::string name, std::vector<std::string> setting);

	bool GetSetting(std::string name, std::string& value);
	bool GetSetting(std::string name, std::vector<std::string>& value);
private:
	std::map<std::string, std::string> settings;
	std::map<std::string, std::vector<std::string>> multiple_settings;
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
	static bool GetSetting(std::string name, lng& value, PGSettings* extra_setting = nullptr);
	static bool GetSetting(std::string name, double& value, PGSettings* extra_setting = nullptr);
	static bool GetSetting(std::string name, std::string& value, PGSettings* extra_setting = nullptr) { return GetInstance()->_GetSetting(name, value, extra_setting); }
private:
	bool _GetSetting(std::string name, std::string& value, PGSettings* extra_setting = nullptr);

	PGSettingsManager();

	PGSettings default_settings;
	PGSettings user_settings;

	std::map<std::string, PGSettings> styles;
};

