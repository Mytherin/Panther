#import <Cocoa/Cocoa.h>
#import "PGView.h"

#include "windowfunctions.h"
#include "xml.h"
#include "c.h"
#include "findresults.h"
#include "scheduler.h"

#include "settings.h"
#include "keybindings.h"
#include "workspace.h"
#include "globalsettings.h"

// Compilation: clang++ -framework Cocoa -fobjc-arc -lobjc test.mm AppDelegate.mm -o test

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm -o main

int main(int argc, const char *argv[])
{
    NSApplication *application = [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    //[[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:application topLevelObjects:&tl];
            
    PGLanguageManager::AddLanguage(new CLanguage());
    PGLanguageManager::AddLanguage(new XMLLanguage());
    PGLanguageManager::AddLanguage(new FindResultsLanguage());

    PGSettingsManager::Initialize();
    PGKeyBindingsManager::Initialize();
    PGGlobalSettings::Initialize("globalsettings.json");

    Scheduler::Initialize();
    Scheduler::SetThreadCount(2);

    // load a workspace
    nlohmann::json& settings = PGGlobalSettings::GetSettings();
    if (settings.count("workspaces") == 0 || !settings["workspaces"].is_array()) {
        // no known workspaces in the settings, initialize the settings
        settings["workspaces"] = nlohmann::json::array();
    }
    lng active_workspace = 0;
    if (settings.count("active_workspace") != 0 && settings["active_workspace"].is_number()) {
        active_workspace = std::min(std::max(0LL, settings["active_workspace"].get<lng>()), (lng)(settings["workspaces"].array().size() - 1));
    } else {
        settings["active_workspace"] = 0;
        active_workspace = 0;
    }
    if (settings["workspaces"].array().size() == 0) {
        // no known workspaces, add one
        settings["workspaces"][0] = "workspace.json";
        settings["active_workspace"] = 0;
        active_workspace = 0;
    }

    std::string workspace_path = settings["workspaces"][active_workspace];
    // load the active workspace
    PGWorkspace* workspace = new PGWorkspace();
    workspace->LoadWorkspace(workspace_path);

    auto windows = workspace->GetWindows();
    for (auto it = windows.begin(); it != windows.end(); it++) {
        ShowWindow(*it);
    }
/*
    TextFile* textfile = new TextFile(nullptr);
    std::vector<std::shared_ptr<TextFile>> files;
    files.push_back(std::shared_ptr<TextFile>(textfile));

    PGWindowHandle window = PGCreateWindow(files);*/
    //ShowWindow(window);

    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];                                                  // Call the Apps Run method

    return 0;       // App Never gets here.*/
}


