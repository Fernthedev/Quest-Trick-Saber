#pragma once

#include <unordered_set>
#include "main.hpp"
#include "PluginConfig.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/typedefs-array.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rigidbody.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/MeshFilter.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "GlobalNamespace/SaberTrailRenderer.hpp"
#include "GlobalNamespace/SaberType.hpp"
#include "GlobalNamespace/SetSaberGlowColor.hpp"
#include "GlobalNamespace/SetSaberFakeGlowColor.hpp"
#include "GlobalNamespace/SaberTypeObject.hpp"
#include "GlobalNamespace/SaberModelContainer.hpp"
#include "GlobalNamespace/SaberModelController.hpp"
#include "GlobalNamespace/SaberTrail.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/SaberMovementData.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "SaberTrickTrail.hpp"
#include <string>

static const std::string saberPrefix = "trick_saber_";

class SaberTrickModel {
  public:
    UnityEngine::Rigidbody* Rigidbody = nullptr;
    UnityEngine::GameObject* SaberGO;  // GameObject
    UnityEngine::Transform* SpinT;   // Transform

    SaberTrickModel(UnityEngine::GameObject* SaberModel, int st) : saberType(st) {
        CRASH_UNLESS(SaberModel);
        getLogger().debug("SaberTrickModel construction!");

        static auto* tSaber = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "Saber"));
        RealSaber = SaberModel->GetComponentsInParent<GlobalNamespace::Saber*>(true)->values[0];

        if (!AttachForSpin && !SpinIsRelativeToVRController) {
            SpinT = RealT = RealSaber->get_transform();
        } else {
            SpinT = RealT = SaberModel->get_transform();
        }


        if (getPluginConfig().EnableTrickCutting.GetValue()) {
            SaberGO = RealModel = RealSaber->get_gameObject();
            SetupRigidbody(RealModel);
        } else {
            // The one to hide during TrickModel appearances
            RealModel = SaberModel;

            if (TrailFollowsSaberComponent) {
                SaberGO = RealSaber->get_gameObject();
            } else {
                SaberGO = SaberModel;
            }

            TrickModel = UnityEngine::Object::Instantiate(RealModel);
            CRASH_UNLESS(TrickModel);
            TrickModel->set_name(il2cpp_utils::createcsstr(
                    saberPrefix + to_utf8(csstrtostr(SaberModel->get_name())) + "_" + std::to_string((int) saber->saberType->saberType)
                    ));
            getLogger().debug("Trick model name: %s", to_utf8(csstrtostr(TrickModel->get_name())).c_str());

            TrickT = TrickModel->get_transform();
            if (AttachForSpin) SpinT = TrickT;

            if (TrailFollowsSaberComponent) {
                TrickSaber = TrickModel->GetComponent<GlobalNamespace::Saber *>();

                if (!TrickSaber) {
                    TrickSaber = TrickModel->AddComponent<GlobalNamespace::Saber*>();
                    TrickSaber->movementData = GlobalNamespace::SaberMovementData::New_ctor();
                    TrickSaber->saberType = RealSaber->saberType;
                    TrickSaber->saberBladeBottomTransform = RealSaber->saberBladeBottomTransform;
                    TrickSaber->saberBladeTopTransform = RealSaber->saberBladeTopTransform;
                    TrickSaber->handleTransform = RealSaber->handleTransform;
                    TrickSaber->saberBladeBottomPos = RealSaber->saberBladeBottomPos;
                    TrickSaber->saberBladeTopPos = RealSaber->saberBladeTopPos;
                }

                CRASH_UNLESS(TrickSaber);
                getLogger().debug("Trick saber component found");
                CRASH_UNLESS(TrickSaber != RealSaber);
                getLogger().debug("Inserting TrickSaber %p to fakeSabers.", TrickSaber);
                fakeSabers.insert(TrickSaber);

                auto *comps = TrickModel->GetComponents<UnityEngine::Behaviour *>();
                for (int i = 0; i < comps->Length(); i++) {
                    auto *klass = CRASH_UNLESS(il2cpp_functions::object_get_class(comps->values[i]));
                    auto name = il2cpp_utils::ClassStandardName(klass);
                    if (ForbiddenComponents.contains(name)) {
                        getLogger().debug("Destroying component of class %s!", name.c_str());
                        UnityEngine::Object::DestroyImmediate(comps->values[i]);
                    }
                }
            }

            FixSaber(TrickModel);
            SetupRigidbody(TrickModel);

            if (AttachForSpin) {
                auto* vrController = SaberModel->GetComponentsInParent<GlobalNamespace::VRController*>(true)->values[0];
                AttachedP = vrController->get_transform();
            }

            auto* str = CRASH_UNLESS(il2cpp_utils::createcsstr("VRGameCore"));
            auto* vrGameCore = UnityEngine::GameObject::Find(str);
            UnattachedP = vrGameCore->get_transform();

            TrickT->SetParent(UnattachedP);
            TrickModel->SetActive(false);
        }

        getLogger().debug("Leaving SaberTrickModel construction!");
    }

    void SetupRigidbody(UnityEngine::GameObject* model) {

        // il2cpp_utils::LogClass(il2cpp_functions::class_from_system_type(tRigidbody), false);
        Rigidbody = model->GetComponentInChildren<UnityEngine::Rigidbody*>();
        if (!Rigidbody) {
            Rigidbody = model->AddComponent<UnityEngine::Rigidbody*>();
        }

        getLogger().debug("Rigid body pos: (%f %f %f) (%f %f %f)", Rigidbody->get_position().x, Rigidbody->get_position().y, Rigidbody->get_position().z, Rigidbody->get_transform()->get_localPosition().x, Rigidbody->get_transform()->get_localPosition().y, Rigidbody->get_transform()->get_localPosition().z);
        Rigidbody->set_useGravity(false);
        Rigidbody->set_isKinematic(true);

        static auto set_detectCollisions = (function_ptr_t<void, Il2CppObject*, bool>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_detectCollisions"));
        getLogger().debug("set_detectCollisions ptr offset: %lX", asOffset(set_detectCollisions));
        set_detectCollisions(Rigidbody, false);

        static auto set_maxAngVel = (function_ptr_t<void, Il2CppObject*, float>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_maxAngularVelocity"));
        getLogger().debug("set_maxAngVel ptr offset: %lX", asOffset(set_maxAngVel));
        set_maxAngVel(Rigidbody, 800.0f);

        static auto set_interp = (function_ptr_t<void, Il2CppObject*, int>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_interpolation"));
        getLogger().debug("set_interpolation ptr offset: %lX", asOffset(set_interp));
        set_interp(Rigidbody, 1);  // Interpolate

        Array<UnityEngine::Collider*>* colliders = model->GetComponentsInChildren<UnityEngine::Collider*>(true); //CRASH_UNLESS(il2cpp_utils::RunMethod<Array<Il2CppObject*>*>(model, "GetComponentsInChildren", tCollider, true));


        for (int i = 0; i < colliders->Length(); i++) {
            colliders->values[i]->set_enabled(false);
        }
    }

    void FixSaber(UnityEngine::GameObject* newSaberGO) {
        getLogger().debug("Fixing up instantiated Saber gameObject!");

        GlobalNamespace::SaberModelContainer *oldSaberModelContainer = SaberGO->GetComponentsInParent<GlobalNamespace::SaberModelContainer *>(
                false)->values[0];
        GlobalNamespace::SaberModelContainer *newSaberModelContainer = newSaberGO->GetComponent<GlobalNamespace::SaberModelContainer *>();

        if (!newSaberModelContainer) {
            getLogger().debug("Container create");
            newSaberModelContainer = newSaberGO->AddComponent<GlobalNamespace::SaberModelContainer *>();
            CRASH_UNLESS(newSaberModelContainer->saber != TrickSaber);
        }

        auto *diContainer = oldSaberModelContainer->container;
        newSaberModelContainer->container = diContainer;

        GlobalNamespace::SaberTypeObject *_saberTypeObject = oldSaberModelContainer->saber->saberType;
        GlobalNamespace::SaberType saberType = _saberTypeObject->saberType; // CRASH_UNLESS(il2cpp_utils::GetPropertyValue(_saberTypeObject, "saberType"));
        getLogger().debug("saber type: %i", saberType);
//        CRASH_UNLESS(saberType);
        auto *newSaberModelContainerT = newSaberModelContainer->get_transform();
        getLogger().debug("saber container");
        CRASH_UNLESS(newSaberModelContainerT);

        auto *newSaberModelController = newSaberGO->GetComponent<GlobalNamespace::SaberModelController *>();
        getLogger().debug("saber controller");
        CRASH_UNLESS(newSaberModelController);

        auto *origModelController = SaberGO->GetComponentInChildren<GlobalNamespace::SaberModelController *>(); // CRASH_UNLESS(il2cpp_utils::RunMethod(SaberGO, "GetComponent", tSaberModelController));
        auto *colorMgr = origModelController->colorManager; // CRASH_UNLESS(il2cpp_utils::GetFieldValue(origModelController, "_colorManager"));
        getLogger().debug("saber color manager");
        CRASH_UNLESS(colorMgr);
        newSaberModelController->colorManager = colorMgr;

        auto *glows = newSaberModelController->setSaberGlowColors; // CRASH_UNLESS(il2cpp_utils::GetFieldValue<Il2CppArray*>(saberModelController, "_setSaberGlowColors"));
        getLogger().debug("_setSaberGlowColors.length: %u", glows->Length());
        for (int i = 0; i < glows->Length(); i++) {
            GlobalNamespace::SetSaberGlowColor *obj = glows->values[i];

            obj->colorManager = colorMgr;
        }

        auto *fakeGlows = newSaberModelController->setSaberFakeGlowColors;
        getLogger().debug("_setSaberFakeGlowColors.length: %u", fakeGlows->Length());
        for (int i = 0; i < fakeGlows->Length(); i++) {
            GlobalNamespace::SetSaberFakeGlowColor *obj = fakeGlows->values[i];

            obj->colorManager = colorMgr;

        }

        newSaberModelController->saberTrail = origModelController->saberTrail;

        getLogger().debug("New Saber model controller");
        CRASH_UNLESS(newSaberModelController);
        getLogger().debug("Saber model controller transform");
        CRASH_UNLESS(newSaberModelContainerT);
        getLogger().debug("Trick Saber instance");
        CRASH_UNLESS(TrickSaber);
        getLogger().debug("Init");
        try {
            newSaberModelController->Init(newSaberModelContainerT, TrickSaber);
        } catch (il2cpp_utils::RunMethodException& e) {
            getLogger().error("Error stacktrace: %s", to_utf8(csstrtostr(e.ex->stackTraceString)).c_str());
            CRASH_UNLESS(true);
        }
    }

    void PrepareForThrow() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (attachedForSpin) {
            getLogger().error("SaberTrickModel::PrepareForThrow was called between PrepareForSpin and EndSpin!");
            EndSpin();
        }
        if (SaberGO == TrickModel) return;
        auto pos = RealT->get_position();
        auto rot = RealT->get_rotation();
        RealModel->SetActive(false);
        TrickModel->SetActive(true);
        TrickT->set_position(pos);
        TrickT->set_rotation(rot);
        SaberGO = TrickModel;
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(TrickSaber);
    }

    void EndThrow() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (SaberGO == RealModel) return;
        RealModel->SetActive(true);
        TrickModel->SetActive(false);
        SaberGO = RealModel;
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(RealSaber);
    }

    void PrepareForSpin() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (!AttachForSpin || attachedForSpin) return;
        getLogger().info("Attaching for spin.");
        auto pos = RealT->get_position();
        auto rot = RealT->get_rotation();
        RealModel->SetActive(false);
        TrickModel->SetActive(true);
        SpinT->set_parent(AttachedP);
        attachedForSpin = true;
        TrickT->set_position(pos);
        TrickT->set_rotation(rot);
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(TrickSaber);
    }

    void EndSpin() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (!AttachForSpin || !attachedForSpin) return;
        getLogger().info("Unattaching post-spin.");
        SpinT->set_parent(AttachedP);
        attachedForSpin = false;
        TrickModel->SetActive(false);
        RealModel->SetActive(true);
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(TrickSaber);
    }

    void Update() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (SaberGO != TrickModel) return;
        // TODO: bypass the hook entirely?
        TrickSaber->ManualUpdate();
    }



  private:
    UnityEngine::GameObject* RealModel = nullptr;  // GameObject
    UnityEngine::GameObject* TrickModel = nullptr;          // GameObject
    inline static const std::unordered_set<std::string> ForbiddenComponents = {
            "::VRController", "::VRControllersValueSOOffsets"
    };
    int saberType;
    bool attachedForSpin = false;
    GlobalNamespace::Saber* RealSaber = nullptr;   // Saber
    GlobalNamespace::Saber* TrickSaber = nullptr;  // Saber
    UnityEngine::Transform* RealT = nullptr;       // Transform
    UnityEngine::Transform* TrickT = nullptr;      // Transform
    UnityEngine::Transform* UnattachedP = nullptr; // Transform
    UnityEngine::Transform* AttachedP = nullptr;   // Transform

    void _UpdateComponentsWithSaber(GlobalNamespace::Saber* saber) {
        for (auto* type : tBurnTypes) {
            auto* components = UnityEngine::Object::FindObjectsOfType(type);
            for (il2cpp_array_size_t i = 0; i < components->Length(); i++) {
                auto* sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Il2CppObject*>*>(components->values[i], "_sabers"));
                sabers->values[saberType] = saber;
            }
        }

    }
};
