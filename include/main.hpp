#pragma once

// Ignore can't be found
#include <dlfcn.h>
#include "GlobalNamespace/AudioTimeSyncController.hpp"

#include <unordered_set>

#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "GlobalNamespace/SaberClashChecker.hpp"
#include "GlobalNamespace/SaberManager.hpp"

inline int objectCount = 0;

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"

// Beatsaber update settings
// Whether the trail follows the Saber component, or the saber model.
constexpr bool TrailFollowsSaberComponent = true;
// Whether to attach a TrickModel for spin
extern bool AttachForSpin;
// Whether the Spin trick spins the VRController, or relative to it.
extern bool SpinIsRelativeToVRController;

inline ModInfo modInfo;
const Logger& logger();
static GlobalNamespace::AudioTimeSyncController *audioTimeSyncController = nullptr;
static GlobalNamespace::SaberManager *saberManager = nullptr;

inline std::unordered_set<Il2CppObject*> fakeSabers;
inline std::unordered_set<Il2CppReflectionType*> tBurnTypes;

void DisableBurnMarks(int saberType);
void EnableBurnMarks(int saberType);

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "modloader/shared/modloader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"

// Define these functions here so that we can easily read configuration and log information from other files
Configuration &getConfig();
Logger &getLogger();