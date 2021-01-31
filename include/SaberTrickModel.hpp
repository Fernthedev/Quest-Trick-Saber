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

        SaberGO = RealModel = SaberModel;
        SpinT = RealT = RealModel->get_transform();
        RealSaber = RealModel->GetComponent<GlobalNamespace::Saber*>();
        AttachedP = RealSaber->get_transform();


        if (getPluginConfig().EnableTrickCutting.GetValue()) {
            SetupRigidbody(RealModel);
        } else {
            TrickModel = UnityEngine::Object::Instantiate(RealModel);
            CRASH_UNLESS(TrickModel);
            TrickModel->set_name(il2cpp_utils::createcsstr(
                    saberPrefix + to_utf8(csstrtostr(SaberModel->get_name())) + "_" + std::to_string((int) saber->saberType->saberType)
                    ));
            getLogger().debug("Trick model name: %s", to_utf8(csstrtostr(TrickModel->get_name())).c_str());
            FixBasicTrickSaber(TrickModel, basicSaber);
            AddTrickRigidbody();

            TrickT = TrickModel->get_transform();
            if (AttachForSpin) SpinT = TrickT;

            TrickSaber = TrickModel->GetComponent<GlobalNamespace::Saber*>();
            CRASH_UNLESS(TrickSaber != RealSaber);
            getLogger().debug("Inserting TrickSaber %p to fakeSabers.", TrickSaber);
            fakeSabers.insert(TrickSaber);

            static auto* tBehaviour = CRASH_UNLESS(il2cpp_utils::GetSystemType("UnityEngine", "Behaviour"));
            auto* comps = TrickModel->GetComponents<UnityEngine::Behaviour*>();
            for (int i = 0; i < comps->Length(); i++) {
                auto* klass = CRASH_UNLESS(il2cpp_functions::object_get_class(comps->values[i]));
                auto name = il2cpp_utils::ClassStandardName(klass);
                if (ForbiddenComponents.contains(name)) {
                    getLogger().debug("Destroying component of class %s!", name.c_str());
                    UnityEngine::Object::DestroyImmediate(comps->values[i]);
                } else if (name == "::Saber") {
                    TrickSaber = reinterpret_cast<GlobalNamespace::Saber*>(comps->values[i]);
                }
            }

            FixSaber(TrickModel);
            SetupRigidbody(TrickModel);



            auto* str = CRASH_UNLESS(il2cpp_utils::createcsstr("VRGameCore"));
            auto* vrGameCore = UnityEngine::GameObject::Find(str);
            UnattachedP = vrGameCore->get_transform();
            auto* trickModelT = TrickModel->get_transform();

            trickModelT->SetParent(UnattachedP);
            TrickModel->SetActive(false);
        }

        getLogger().debug("Leaving SaberTrickModel construction!");
    }

    void SetupRigidbody(UnityEngine::GameObject* model) {

        // il2cpp_utils::LogClass(il2cpp_functions::class_from_system_type(tRigidbody), false);
        Rigidbody = model->GetComponent<UnityEngine::Rigidbody*>();
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
        GlobalNamespace::SaberModelContainer *newSaberModelContainer = newSaberGO->GetComponentsInParent<GlobalNamespace::SaberModelContainer *>(
                false)->values[0];

        auto *diContainer = oldSaberModelContainer->container;
        newSaberModelContainer->container = diContainer;

        GlobalNamespace::SaberTypeObject *_saberTypeObject = oldSaberModelContainer->saber->saberType;
        GlobalNamespace::SaberType saberType = _saberTypeObject->saberType; // CRASH_UNLESS(il2cpp_utils::GetPropertyValue(_saberTypeObject, "saberType"));
        getLogger().debug("saber type: %i", saberType);
//        CRASH_UNLESS(saberType);
        auto *saberModelContainerT = oldSaberModelContainer->get_transform();
        getLogger().debug("saber container");
        CRASH_UNLESS(saberModelContainerT);

        auto *saberModelController = newSaberGO->GetComponent<GlobalNamespace::SaberModelController *>();
        getLogger().debug("saber controller");
        CRASH_UNLESS(saberModelController);

        auto *origModelController = SaberGO->GetComponent<GlobalNamespace::SaberModelController *>(); // CRASH_UNLESS(il2cpp_utils::RunMethod(SaberGO, "GetComponent", tSaberModelController));
        auto *colorMgr = origModelController->colorManager; // CRASH_UNLESS(il2cpp_utils::GetFieldValue(origModelController, "_colorManager"));
        getLogger().debug("saber color manager");
        CRASH_UNLESS(colorMgr);
        saberModelController->colorManager = colorMgr;

        auto *glows = saberModelController->setSaberGlowColors; // CRASH_UNLESS(il2cpp_utils::GetFieldValue<Il2CppArray*>(saberModelController, "_setSaberGlowColors"));
        getLogger().debug("_setSaberGlowColors.length: %u", glows->Length());
        for (int i = 0; i < glows->Length(); i++) {
            GlobalNamespace::SetSaberGlowColor *obj = glows->values[i];

            obj->colorManager = colorMgr;
        }

        auto *fakeGlows = saberModelController->setSaberFakeGlowColors;
        getLogger().debug("_setSaberFakeGlowColors.length: %u", fakeGlows->Length());
        for (int i = 0; i < fakeGlows->Length(); i++) {
            GlobalNamespace::SetSaberFakeGlowColor *obj = fakeGlows->values[i];

            obj->colorManager = colorMgr;

        }

        saberModelController->Init(saberModelContainerT, TrickSaber);
    }

    void PrepareForThrow() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (attachedForSpin) {
            getLogger().error("SaberTrickModel::PrepareForThrow was called between PrepareForSpin and EndSpin!");
            EndSpin();
        }
        if (SaberGO == TrickModel) return;
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", true));
        auto pos = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(RealT, "position"));
        auto rot = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Quaternion>(RealT, "rotation"));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(TrickT, "position", pos));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(TrickT, "rotation", rot));
        CRASH_UNLESS(il2cpp_utils::RunMethod(RealModel, "SetActive", false));
        SaberGO = TrickModel;
        _UpdateComponentsWithSaber(TrickSaber);
    }

    void EndThrow() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (SaberGO == RealModel) return;
        CRASH_UNLESS(il2cpp_utils::RunMethod(RealModel, "SetActive", true));
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", false));
        SaberGO = RealModel;
        _UpdateComponentsWithSaber(RealSaber);
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
    }

    void EndSpin() {
        if (getPluginConfig().EnableTrickCutting.GetValue()) return;
        if (!AttachForSpin || !attachedForSpin) return;
        getLogger().info("Unattaching post-spin.");
        SpinT->set_parent(AttachedP);
        attachedForSpin = false;
        TrickModel->SetActive(false);
        RealModel->SetActive(true);
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
