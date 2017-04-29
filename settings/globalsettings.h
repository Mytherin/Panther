#pragma once

#include <map>
#include <vector>

#include "utils.h"
#include "json.h"

class PGGlobalSettings {
public:
	static PGGlobalSettings* GetInstance(std::string path = "") {
		static PGGlobalSettings settingsmanager = PGGlobalSettings(path);
		return &settingsmanager;
	}

	static void Initialize(std::string path) { (void)GetInstance(path); }

	static nlohmann::json& GetSettings() { return GetInstance()->settings; }
	static void WriteSettings() { return GetInstance()->_WriteSettings(); }
private:
	PGGlobalSettings(std::string path);

	void _WriteSettings();

	std::string path;
	nlohmann::json settings;
};

