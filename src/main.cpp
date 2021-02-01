#include "../include/main.hpp"
#include "../include/PluginConfig.hpp"
#include "../include/TrickManager.hpp"
#include "ConfigEnums.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/GameSongController.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "GlobalNamespace/GamePause.hpp"
#include "GlobalNamespace/OculusVRHelper.hpp"
#include "GlobalNamespace/SaberBurnMarkArea.hpp"
#include "GlobalNamespace/SaberBurnMarkSparkles.hpp"

#include "GlobalNamespace/ObstacleSaberSparkleEffectManager.hpp"
#include "custom-types/shared/register.hpp"

#include "GlobalNamespace/GameScenesManager.hpp"
#include "System/Action.hpp"
#include "System/Action_1.hpp"
#include "Zenject/DiContainer.hpp"
#include "GlobalNamespace/ScenesTransitionSetupDataSO.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "HMUI/Touchable.hpp"

#include "questui/shared/CustomTypes/Components/ExternalComponents.hpp"
#include "bs-utils/shared/utils.hpp"

#include <cstdlib>

#include <string>

#ifdef HAS_CODEGEN

#define AddConfigValueIncrementEnum(parent, enumConfigValue, enumClass, enumMap) BeatSaberUI::CreateIncrementSetting(parent, enumConfigValue.GetName() + " " + enumMap.at((int) enumConfigValue.GetValue()), 0, 1, (float) enumConfigValue.GetValue(), 0,(int) enumMap.size() - 1, il2cpp_utils::MakeDelegate<UnityEngine::Events::UnityAction_1<float>*>(classof(UnityEngine::Events::UnityAction_1<float>*), (void*)nullptr, +[](float value) { enumConfigValue.SetValue((int)value); }))

#endif

using namespace GlobalNamespace;
using namespace QuestUI;


// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static auto* logger = new Logger(modInfo);
    return *logger;
}



Configuration& getConfig() {
    static Configuration configuration(modInfo);
    return configuration;
}

extern "C" void setup(ModInfo& info) {
    info.id      = "TrickSaber";
    info.version = "0.3.1";
    modInfo      = info;
    getConfig().Load();
    getPluginConfig().Init(info);
    getConfig().Reload();
    getConfig().Write();

    getLogger().info("Leaving setup!");
}

Saber* FakeSaber = nullptr;
Saber* RealSaber = nullptr;
TrickManager leftSaber;
TrickManager rightSaber;


bool AttachForSpin;
bool SpinIsRelativeToVRController;
void UpdateForConfig() {
    AttachForSpin = TrailFollowsSaberComponent && !getPluginConfig().EnableTrickCutting.GetValue();
    SpinIsRelativeToVRController = AttachForSpin;
}


MAKE_HOOK_OFFSETLESS(SceneManager_Internal_SceneLoaded, void, UnityEngine::SceneManagement::Scene scene, UnityEngine::SceneManagement::LoadSceneMode mode) {
    getLogger().info("SceneManager_Internal_SceneLoaded");
    if (auto* nameOpt = scene.get_name()) {
        auto* name = nameOpt;
        auto str = to_utf8(csstrtostr(name));
        getLogger().debug("Scene name internal: %s", str.c_str());
        // sabersEnabled = (str == "GameCore");
    }
    FakeSaber = nullptr;
    RealSaber = nullptr;
    TrickManager::StaticClear();
    leftSaber.Clear();
    rightSaber.Clear();
    SceneManager_Internal_SceneLoaded(scene, mode);
    fakeSabers.clear();
}

MAKE_HOOK_OFFSETLESS(GameScenesManager_PushScenes, void, GlobalNamespace::GameScenesManager* self, GlobalNamespace::ScenesTransitionSetupDataSO* scenesTransitionSetupData,
                     float minDuration, System::Action* afterMinDurationCallback,
                     System::Action_1<Zenject::DiContainer*>* finishCallback) {
    getLogger().debug("GameScenesManager_PushScenes");
    GameScenesManager_PushScenes(self, scenesTransitionSetupData, minDuration, afterMinDurationCallback, finishCallback);
    getConfig().Reload();

    if (getPluginConfig().EnableTrickCutting.GetValue() || getPluginConfig().SlowmoDuringThrow.GetValue()) {
        bs_utils::Submission::disable(modInfo);
    } else {
        bs_utils::Submission::enable(modInfo);
    }

    UpdateForConfig();

    objectCount = 0;

    getLogger().debug("Leaving GameScenesManager_PushScenes");
}

MAKE_HOOK_OFFSETLESS(SaberManager_Start, void, SaberManager* self) {
    SaberManager_Start(self);
    saberManager = self;
    getLogger().debug("SaberManager_Start");
}

MAKE_HOOK_OFFSETLESS(Saber_Start, void, Saber* self) {
    getLogger().debug("Saber_Start");
    Saber_Start(self);
    getLogger().debug("Saber_Start original called name: %s", to_utf8(csstrtostr(self->get_name())).c_str());

    if (to_utf8(csstrtostr(self->get_name())).starts_with(saberPrefix)) {
        getLogger().debug("Ignoring trick start");
        return;
    }

    // saber->sabertypeobject->sabertype
    SaberType saberType = self->saberType->saberType;
    getLogger().debug("SaberType: %i", saberType);

    auto *vrControllers = UnityEngine::Resources::FindObjectsOfTypeAll<VRController*>();


    getLogger().info("VR controllers: %i", vrControllers->Length());

    for (int i = 0; i<vrControllers->Length(); i++) {
        auto* controller = vrControllers->values[i];

    }




    if ((int) saberType == 0) {
        getLogger().debug("Left?");
//        leftSaber.VRController = vrControllersManager->get_node();
        leftSaber.Saber = self;
        leftSaber._isLeftSaber = true;
        leftSaber.other = &rightSaber;
        leftSaber.Start();
    } else {
        getLogger().debug("Right?");
//        rightSaber.VRController = vrControllersManager->rightVRController;
        rightSaber.Saber = self;
        rightSaber.other = &leftSaber;
        rightSaber.Start();
    }
    RealSaber = self;
}

MAKE_HOOK_OFFSETLESS(Saber_ManualUpdate, void, Saber* self) {
    Saber_ManualUpdate(self);
    if (self == leftSaber.Saber) {
        leftSaber.Update();
    } else if (self == rightSaber.Saber) {
        // rightSaber.LogEverything();
        rightSaber.Update();
    }
}


// TODO: remove
void DisableBurnMarks(int saberType) {
    if (TrailFollowsSaberComponent) return;
    if (!FakeSaber) {
        auto* core = UnityEngine::GameObject::Find(il2cpp_utils::createcsstr("GameCore"));
        FakeSaber = core->AddComponent<Saber*>();

        FakeSaber->set_enabled(false);
        fakeSabers.insert(FakeSaber);

        getLogger().info("FakeSaber.isActiveAndEnabled: %i",FakeSaber->get_isActiveAndEnabled());

        auto* saberTypeObj = RealSaber->saberType;
        FakeSaber->saberType = saberTypeObj;
        getLogger().info("FakeSaber SaberType: %i", FakeSaber->saberType);
    }
    for (auto* type : tBurnTypes) {
        auto *components = UnityEngine::Object::FindObjectsOfType(type);
        for (il2cpp_array_size_t i = 0; i < components->Length(); i++) {
            auto *sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Saber *> *>(components->values[i], "_sabers"));
            sabers->values[saberType] = FakeSaber;
        }
    }
    getLogger().debug("Leaving DisableBurnMarks");
}

void EnableBurnMarks(int saberType) {
    if (TrailFollowsSaberComponent) return;
    for (auto* type : tBurnTypes) {
        auto *components = UnityEngine::Object::FindObjectsOfType(type);
        for (int i = 0; i < components->Length(); i++) {

            auto *sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Saber *> *>(components->values[i], "_sabers"));
            sabers->values[saberType] = saberType ? saberManager->rightSaber : saberManager->leftSaber;
        }
    }
    /*
    for (auto *type : tBurnTypes) {
        auto *components = UnityEngine::Object::FindObjectsOfType(type);
        for (int i = 0; i < components->Length(); i++) {
            getLogger().debug("Burn Type: %s", components->values[i]->get_name());
            auto *sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Saber *> *>(components->values[i], "_sabers"));
            sabers->values[saberType] = saberType ? saberManager->rightSaber : saberManager->leftSaber;
        }
    }
     */
}


MAKE_HOOK_OFFSETLESS(FixedUpdate, void, GlobalNamespace::OculusVRHelper* self) {
    FixedUpdate(self);
    TrickManager::StaticFixedUpdate();
}

MAKE_HOOK_OFFSETLESS(Pause, void, GlobalNamespace::GamePause* self) {
    leftSaber.PauseTricks();
    rightSaber.PauseTricks();
    Pause(self);
    TrickManager::StaticPause();
    getLogger().debug("pause: %i", self->pause);
}

MAKE_HOOK_OFFSETLESS(Resume, void, GlobalNamespace::GamePause* self) {
    Resume(self);
    TrickManager::StaticResume();
    leftSaber.ResumeTricks();
    rightSaber.ResumeTricks();
    getLogger().debug("pause: %i", self->pause);
}

MAKE_HOOK_OFFSETLESS(AudioTimeSyncController_Start, void, AudioTimeSyncController* self) {
    AudioTimeSyncController_Start(self);
    audioTimeSyncController = self;
    getLogger().debug("audio time controller: %i");
}

MAKE_HOOK_OFFSETLESS(SaberClashChecker_AreSabersClashing, bool, SaberClashChecker* self) {
    bool val = SaberClashChecker_AreSabersClashing(self);

    return (rightSaber.doClashEffect && leftSaber.doClashEffect) && val;
}

MAKE_HOOK_OFFSETLESS(VRController_Update, void, GlobalNamespace::VRController* self) {
    VRController_Update(self);

    if (self->get_node() == UnityEngine::XR::XRNode::LeftHand) {
        leftSaber.VRController = self;
    }

    if (self->get_node() == UnityEngine::XR::XRNode::RightHand) {
        rightSaber.VRController = self;
    }
}


MAKE_HOOK_OFFSETLESS(SpawnNote, void, Il2CppObject* self, Il2CppObject* noteData, float cutDirectionAngleOffset) {
    objectCount++;
    getLogger().debug("Object count note increase %d", objectCount);

    SpawnNote(self, noteData, cutDirectionAngleOffset);
}

MAKE_HOOK_OFFSETLESS(SpawnBomb, void, Il2CppObject* self, Il2CppObject* noteData) {
    objectCount++;
    getLogger().debug("Object count bomb increase %d", objectCount);

    SpawnBomb(self, noteData);
}

MAKE_HOOK_OFFSETLESS(NoteCut, void, Il2CppObject* self, Il2CppObject* noteController, Il2CppObject* noteCutInfo) {
    objectCount--;
    getLogger().debug("Object count note cut decrease %d", objectCount);

    if (objectCount < 0) objectCount = 0;

    NoteCut(self, noteController, noteCutInfo);
}

MAKE_HOOK_OFFSETLESS(NoteMissed, void, Il2CppObject* self, Il2CppObject* noteController) {
    objectCount--;
    getLogger().debug("Object count decrease %d", objectCount);

    if (objectCount < 0) objectCount = 0;

    NoteMissed(self, noteController);
}


void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    getLogger().info("DidActivate: %p, %d, %d, %d", self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if(firstActivation) {
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
        UnityEngine::GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());



        auto* textGrid = container;
//        textGrid->set_spacing(1);

        BeatSaberUI::CreateText(textGrid->get_transform(), "TrickSaber settings. Restart to avoid crashes or side-effects.");

        BeatSaberUI::CreateText(textGrid->get_transform(), "Settings are saved when changed.");

//        buttonsGrid->set_spacing(1);

        auto* boolGrid = container;

        BeatSaberUI::CreateText(boolGrid->get_transform(), "Toggles and switches for buttons.", false);


        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().ReverseTrigger)->get_gameObject(),"Inverts the trigger button");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().ReverseButtonOne)->get_gameObject(),"Inverts the button one toggle.");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().ReverseButtonTwo)->get_gameObject(),"Inverts the button two toggle.");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().ReverseGrip)->get_gameObject(),"Inverts the grip toggle.");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().ReverseThumbstick)->get_gameObject(),"Inverts the thumbstick direction.");

        BeatSaberUI::CreateText(boolGrid->get_transform(), "Preferences.", false);
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().NoTricksWhileNotes)->get_gameObject(),"Doesn't allow tricks while notes are on screen");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().VibrateOnReturn)->get_gameObject(),"Makes the controller vibrate when it returns from being thrown");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().IsVelocityDependent)->get_gameObject(),"Makes the spin speed velocity dependent.");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().EnableTrickCutting)->get_gameObject(),"Allows for physics to apply with the tricks. DOES NOT WORK AND IS VERY BROKEN.");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().CompleteRotationMode)->get_gameObject(),"Allows for the spin rotation to go all directions.");
        BeatSaberUI::AddHoverHint(AddConfigValueToggle(boolGrid->get_transform(), getPluginConfig().SlowmoDuringThrow)->get_gameObject(),"Makes the thrown saber act slow-mo like.");

        auto* floatGrid = container;
        BeatSaberUI::CreateText(floatGrid->get_transform(), "Numbers and math. Threshold values", false);
//        floatGrid->set_spacing(1);

        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().GripThreshold, 2, 0.01, 0, 1)->get_gameObject(),"The deadzone or minimum amount of input required to trigger the grip.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().ControllerSnapThreshold, 2, 0.01, 0, 1)->get_gameObject(),"The deadzone or minimum amount of input required for the controller to snap.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().ThumbstickThreshold, 2, 0.01, 0, 1)->get_gameObject(),"The deadzone or minimum amount of input required to trigger the thumbstick.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().TriggerThreshold, 2, 0.01, 0, 1)->get_gameObject(),"The deadzone or minimum amount of input required to trigger.");

        BeatSaberUI::CreateText(floatGrid->get_transform(), "Speed and velocity manipulation", false);

        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().SpinSpeed, 1, 0.1, 0, 10)->get_gameObject(),"The speed the saber spins at.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().ThrowVelocity, 1, 0.1, 0, 10)->get_gameObject(),"The velocity of the saber when you throw it.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().ReturnSpeed, 1, 0.1, 0, 10)->get_gameObject(),"The speed in which the saber returns to your hand.");


        BeatSaberUI::CreateText(floatGrid->get_transform(), "Technical numbers, please avoid.", false);
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().SlowmoStepAmount, 1, 0.1, 0, 10)->get_gameObject(),"The slow motion time scale amount.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().SlowmoAmount, 2, 0.1, 0, 10)->get_gameObject(),"The intensity of the slow motion.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().VelocityBufferSize, 0, 1, 0, 10)->get_gameObject(),"Technical number for the size of the list that holds velocity throughout time.");

        auto* actionGrid = container;
        BeatSaberUI::CreateText(actionGrid->get_transform(), "Actions Remapping (UI is very funky here)", false);
//        actionGrid->set_name(il2cpp_utils::createcsstr("Actions"));
//        actionGrid->set_spacing(1);

        BeatSaberUI::AddHoverHint(AddConfigValueIncrementEnum(actionGrid->get_transform(), getPluginConfig().TriggerAction, TrickAction, ACTION_NAMES)->get_gameObject(),"The action the trigger performs.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementEnum(actionGrid->get_transform(), getPluginConfig().ButtonOneAction, TrickAction, ACTION_NAMES)->get_gameObject(),"The action the button one performs.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementEnum(actionGrid->get_transform(), getPluginConfig().ButtonTwoAction, TrickAction, ACTION_NAMES)->get_gameObject(),"The action the button two performs.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementEnum(actionGrid->get_transform(), getPluginConfig().GripAction, TrickAction, ACTION_NAMES)->get_gameObject(),"The action the grip button performs.");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementEnum(actionGrid->get_transform(), getPluginConfig().ThumbstickAction, TrickAction, ACTION_NAMES)->get_gameObject(),"The action the thumbstick performs.");

        auto* miscGrid = container;
        BeatSaberUI::CreateText(miscGrid->get_transform(), "Misc", false);
//        miscGrid->set_spacing(1);

        BeatSaberUI::AddHoverHint(AddConfigValueIncrementEnum(miscGrid->get_transform(), getPluginConfig().SpinDirection, TrickAction, SPIN_DIR_NAMES)->get_gameObject(),"The direction of spinning. Still dependent on reverse button");
        BeatSaberUI::AddHoverHint(AddConfigValueIncrementEnum(miscGrid->get_transform(), getPluginConfig().ThumbstickDirection, TrickAction, THUMBSTICK_DIR_NAMES)->get_gameObject(),"The direction of the thumbsticks for tricks. ");

    }
}

void DidDeActivate(HMUI::ViewController* self,bool removedFromHierarchy, bool screenSystemDisabling){
    getLogger().info("Saving config because of menu");
    getConfig().Write();
    getConfig().Reload();
}

extern "C" void load() {
    il2cpp_functions::Init();
    UpdateForConfig();
    // TODO: config menus
    getLogger().info("Installing hooks...");

    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_SceneLoaded, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_SceneLoaded", 2));
    INSTALL_HOOK_OFFSETLESS(getLogger(), GameScenesManager_PushScenes, il2cpp_utils::FindMethodUnsafe("", "GameScenesManager", "PushScenes", 4));
    INSTALL_HOOK_OFFSETLESS(getLogger(), Saber_Start, il2cpp_utils::FindMethod("", "Saber", "Start"));
    INSTALL_HOOK_OFFSETLESS(getLogger(), Saber_ManualUpdate, il2cpp_utils::FindMethod("", "Saber", "ManualUpdate"));

    INSTALL_HOOK_OFFSETLESS(getLogger(), FixedUpdate, il2cpp_utils::FindMethod("", "OculusVRHelper", "FixedUpdate"));
    // INSTALL_HOOK_OFFSETLESS(LateUpdate, il2cpp_utils::FindMethod("", "SaberBurnMarkSparkles", "LateUpdate"));

    INSTALL_HOOK_OFFSETLESS(getLogger(), Pause, il2cpp_utils::FindMethod("", "GamePause", "Pause"));
    INSTALL_HOOK_OFFSETLESS(getLogger(), Resume, il2cpp_utils::FindMethod("", "GamePause", "Resume"));

    INSTALL_HOOK_OFFSETLESS(getLogger(), AudioTimeSyncController_Start, il2cpp_utils::FindMethod("", "AudioTimeSyncController", "Start"));
    INSTALL_HOOK_OFFSETLESS(getLogger(), SaberManager_Start, il2cpp_utils::FindMethod("", "SaberManager", "Start"));

    INSTALL_HOOK_OFFSETLESS(getLogger(), SaberClashChecker_AreSabersClashing, il2cpp_utils::FindMethodUnsafe("", "SaberClashChecker", "AreSabersClashing", 1));
//    INSTALL_HOOK_OFFSETLESS(getLogger(), SaberClashEffect_Disable, il2cpp_utils::FindMethod("", "SaberClashEffect", "OnDisable"));

    INSTALL_HOOK_OFFSETLESS(getLogger(), VRController_Update, il2cpp_utils::FindMethod("", "VRController", "Update"));


    INSTALL_HOOK_OFFSETLESS(getLogger(), SpawnNote, il2cpp_utils::FindMethodUnsafe("", "BeatmapObjectSpawnController", "SpawnBasicNote", 2));
    INSTALL_HOOK_OFFSETLESS(getLogger(), SpawnBomb, il2cpp_utils::FindMethodUnsafe("", "BeatmapObjectSpawnController", "SpawnBombNote", 1));
    INSTALL_HOOK_OFFSETLESS(getLogger(), NoteMissed, il2cpp_utils::FindMethodUnsafe("", "BeatmapObjectManager", "HandleNoteWasMissed", 1));
    INSTALL_HOOK_OFFSETLESS(getLogger(), NoteCut, il2cpp_utils::FindMethodUnsafe("", "BeatmapObjectManager", "HandleNoteWasCut", 2));

//    INSTALL_HOOK_OFFSETLESS(getLogger(), VRController_Update, il2cpp_utils::FindMethodUnsafe("", "GameSongController", "StartSong", 1));

    tBurnTypes.insert(csTypeOf(GlobalNamespace::SaberBurnMarkArea*));
    tBurnTypes.insert(csTypeOf(GlobalNamespace::SaberBurnMarkSparkles*));
    tBurnTypes.insert(csTypeOf(GlobalNamespace::ObstacleSaberSparkleEffectManager*));

    getLogger().info("Registering custom types");
    custom_types::Register::RegisterType<TrickSaber::TrickSaberTrailData>();
    getLogger().info("Registered types");

    getLogger().info("Installed all hooks!");

    getLogger().info("Starting Settings UI installation...");

    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    getLogger().info("Successfully installed Settings UI!");
}
