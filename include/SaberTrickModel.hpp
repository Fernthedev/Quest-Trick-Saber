#pragma once

#include <unordered_set>
#include "main.hpp"
#include "PluginConfig.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils.h"

class SaberTrickModel {
  public:
    Il2CppObject* Rigidbody = nullptr;
    Il2CppObject* SaberGO;  // GameObject
    Il2CppObject* SpinT;   // Transform

    SaberTrickModel(Il2CppObject* SaberModel, int st) : saberType(st) {
        CRASH_UNLESS(SaberModel);
        logger().debug("SaberTrickModel construction!");

        if (!Transform_SetParent) {
            Transform_SetParent = CRASH_UNLESS(il2cpp_utils::FindMethodUnsafe("UnityEngine", "Transform", "set_parent", 1));
        }

        static auto* tSaber = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "Saber"));
        RealSaber = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberModel, "GetComponentInParent", tSaber));
        
        if (!AttachForSpin && !SpinIsRelativeToVRController) {
            SpinT = RealT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(RealSaber, "transform"));
        } else {
            SpinT = RealT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(SaberModel, "transform"));
        }

        if (PluginConfig::Instance().EnableTrickCutting) {
            SaberGO = RealModel = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(RealSaber, "gameObject"));
            SetupRigidbody(RealModel);
        } else {
            // The one to hide during TrickModel appearances
            RealModel = SaberModel;

            if (TrailFollowsSaberComponent) {
                SaberGO = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(RealSaber, "gameObject"));
            } else {
                SaberGO = SaberModel;
            }

            TrickModel = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "Object", "Instantiate", SaberGO));
            CRASH_UNLESS(TrickModel);

            TrickT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(TrickModel, "transform"));
            if (AttachForSpin) SpinT = TrickT;

            if (TrailFollowsSaberComponent) {
                TrickSaber = CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "GetComponent", tSaber));
                CRASH_UNLESS(TrickSaber);
                CRASH_UNLESS(TrickSaber != RealSaber);
                logger().debug("Inserting TrickSaber %p to fakeSabers.", TrickSaber);
                fakeSabers.insert(TrickSaber);

                static auto* tBehaviour = CRASH_UNLESS(il2cpp_utils::GetSystemType("UnityEngine", "Behaviour"));
                auto* comps = CRASH_UNLESS(il2cpp_utils::RunMethod<Array<Il2CppObject*>*>(TrickModel, "GetComponents", tBehaviour));
                static auto* destroy = CRASH_UNLESS(il2cpp_utils::FindMethodUnsafe("UnityEngine", "Object", "DestroyImmediate", 1));
                for (il2cpp_array_size_t i = 0; i < comps->Length(); i++) {
                    auto* klass = CRASH_UNLESS(il2cpp_functions::object_get_class(comps->values[i]));
                    auto name = il2cpp_utils::ClassStandardName(klass);
                    if (ForbiddenComponents.contains(name)) {
                        logger().debug("Destroying component of class %s!", name.c_str());
                        CRASH_UNLESS(il2cpp_utils::RunMethod(nullptr, destroy, comps->values[i]));
                    }
                }
            }

            FixSaber(TrickModel);
            SetupRigidbody(TrickModel);

            if (AttachForSpin) {
                static auto* tVRController = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "VRController"));
                auto* vrController = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberModel, "GetComponentInParent", tVRController));
                AttachedP = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(vrController, "transform"));
            }

            auto* str = CRASH_UNLESS(il2cpp_utils::createcsstr("VRGameCore"));
            auto* vrGameCore = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "GameObject", "Find", str));
            UnattachedP = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(vrGameCore, "transform"));

            ListGameObjects(TrickModel);

            CRASH_UNLESS((il2cpp_utils::RunMethod<Il2CppObject*, false>(TrickT, Transform_SetParent, UnattachedP)));
            CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", false));
        }
        
        logger().debug("Leaving SaberTrickModel construction!");
    }

    void SetupRigidbody(Il2CppObject* model) {
        static auto* tRigidbody = CRASH_UNLESS(il2cpp_utils::GetSystemType("UnityEngine", "Rigidbody"));
        // il2cpp_utils::LogClass(il2cpp_functions::class_from_system_type(tRigidbody), false);
        Rigidbody = CRASH_UNLESS(il2cpp_utils::RunMethod(model, "GetComponentInChildren", tRigidbody));
        if (!Rigidbody) {
            Rigidbody = CRASH_UNLESS(il2cpp_utils::RunMethod(model, "AddComponent", tRigidbody));
        }
        CRASH_UNLESS(Rigidbody);

        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(Rigidbody, "useGravity", false));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(Rigidbody, "isKinematic", true));

        static auto set_detectCollisions = (function_ptr_t<void, Il2CppObject*, bool>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_detectCollisions"));
        logger().debug("set_detectCollisions ptr offset: %lX", asOffset(set_detectCollisions));
        set_detectCollisions(Rigidbody, false);

        static auto set_maxAngVel = (function_ptr_t<void, Il2CppObject*, float>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_maxAngularVelocity"));
        logger().debug("set_maxAngVel ptr offset: %lX", asOffset(set_maxAngVel));
        set_maxAngVel(Rigidbody, 800.0f);

        static auto set_interp = (function_ptr_t<void, Il2CppObject*, int>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_interpolation"));
        logger().debug("set_interpolation ptr offset: %lX", asOffset(set_interp));
        set_interp(Rigidbody, 1);  // Interpolate

        auto* tCollider = CRASH_UNLESS(il2cpp_utils::GetSystemType("UnityEngine", "Collider"));
        auto* colliders = CRASH_UNLESS(
            il2cpp_utils::RunMethod<Array<Il2CppObject*>*>(model, "GetComponentsInChildren", tCollider, true));
        for (size_t i = 0; i < colliders->Length(); i++) {
            CRASH_UNLESS(il2cpp_utils::SetPropertyValue(colliders->values[i], "enabled", false));
        }
    }

    void FixSaber(Il2CppObject* newSaberGO) {
        logger().debug("Fixing up instantiated Saber gameObject!");

        // Mostly only need to set [Inject] fields
        static auto* tSaberModelContainer = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "SaberModelContainer"));
        auto* oldSaberModelContainer = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberGO, "GetComponentInParent", tSaberModelContainer));
        auto* saberModelContainer = CRASH_UNLESS(il2cpp_utils::RunMethod(newSaberGO, "GetComponentInParent", tSaberModelContainer));
        auto* diContainer = CRASH_UNLESS(il2cpp_utils::GetFieldValue(oldSaberModelContainer, "_container"));
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(saberModelContainer, "_container", diContainer));

        static auto* tSaberModelController = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "SaberModelController"));
        auto* origModelController = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberGO, "GetComponentInChildren", tSaberModelController));
        CRASH_UNLESS(origModelController);
        auto* colorMgr = CRASH_UNLESS(il2cpp_utils::GetFieldValue(origModelController, "_colorManager"));
        CRASH_UNLESS(colorMgr);

        // TODO: instead of using specific variables, go through all components and set any "_colorManager" field we can find?
        auto* saberModelController = CRASH_UNLESS(il2cpp_utils::RunMethod(newSaberGO, "GetComponentInChildren", tSaberModelController));
        CRASH_UNLESS(saberModelController);
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(saberModelController, "_colorManager", colorMgr));

        auto* glows = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Il2CppArray*>(saberModelController, "_setSaberGlowColors"));
        logger().debug("_setSaberGlowColors.length: %u", il2cpp_functions::array_length(glows));
        for (il2cpp_array_size_t i = 0; i < il2cpp_functions::array_length(glows); i++) {
            auto* obj = il2cpp_array_get(glows, Il2CppObject*, i);
            CRASH_UNLESS(il2cpp_utils::SetFieldValue(obj, "_colorManager", colorMgr));
        }

        auto* fakeGlows = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Il2CppArray*>(saberModelController, "_setSaberFakeGlowColors"));
        logger().debug("_setSaberFakeGlowColors.length: %u", il2cpp_functions::array_length(fakeGlows));
        for (il2cpp_array_size_t i = 0; i < il2cpp_functions::array_length(fakeGlows); i++) {
            auto* obj = il2cpp_array_get(fakeGlows, Il2CppObject*, i);
            CRASH_UNLESS(il2cpp_utils::SetFieldValue(obj, "_colorManager", colorMgr));
        }

        auto* saberT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(saberModelContainer, "transform"));
        CRASH_UNLESS(saberT);
        CRASH_UNLESS(il2cpp_utils::RunMethod(saberModelController, "Init", saberT, TrickSaber));
    }

    void PrepareForThrow() {
        if (PluginConfig::Instance().EnableTrickCutting) return;
        if (attachedForSpin) {
            logger().error("SaberTrickModel::PrepareForThrow was called between PrepareForSpin and EndSpin!");
            EndSpin();
        }
        if (SaberGO == TrickModel) return;
        auto pos = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(RealT, "position"));
        auto rot = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Quaternion>(RealT, "rotation"));
        CRASH_UNLESS(il2cpp_utils::RunMethod(RealModel, "SetActive", false));
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", true));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(TrickT, "position", pos));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(TrickT, "rotation", rot));
        SaberGO = TrickModel;
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(TrickSaber);
    }

    void EndThrow() {
        if (PluginConfig::Instance().EnableTrickCutting) return;
        if (SaberGO == RealModel) return;
        CRASH_UNLESS(il2cpp_utils::RunMethod(RealModel, "SetActive", true));
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", false));
        SaberGO = RealModel;
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(RealSaber);
    }

    void PrepareForSpin() {
        if (PluginConfig::Instance().EnableTrickCutting) return;
        if (!AttachForSpin || attachedForSpin) return;
        logger().info("Attaching for spin.");
        auto pos = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(RealT, "position"));
        auto rot = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Quaternion>(RealT, "rotation"));
        CRASH_UNLESS(il2cpp_utils::RunMethod(RealModel, "SetActive", false));
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", true));
        CRASH_UNLESS((il2cpp_utils::RunMethod<Il2CppObject*, false>(SpinT, Transform_SetParent, AttachedP)));
        attachedForSpin = true;
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(TrickT, "position", pos));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(TrickT, "rotation", rot));
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(TrickSaber);
    }

    void EndSpin() {
        if (PluginConfig::Instance().EnableTrickCutting) return;
        if (!AttachForSpin || !attachedForSpin) return;
        logger().info("Unattaching post-spin.");
        CRASH_UNLESS((il2cpp_utils::RunMethod<Il2CppObject*, false>(SpinT, Transform_SetParent, UnattachedP)));
        attachedForSpin = false;
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", false));
        CRASH_UNLESS(il2cpp_utils::RunMethod(RealModel, "SetActive", true));
        if (TrailFollowsSaberComponent) _UpdateComponentsWithSaber(TrickSaber);
    }

    void Update() {
        if (PluginConfig::Instance().EnableTrickCutting) return;
        if (SaberGO != TrickModel && !attachedForSpin) return;
        // TODO: bypass the hook entirely?
        logger().info("TrickModel manual update.");
        static auto* update = CRASH_UNLESS(il2cpp_utils::FindMethod("", "Saber", "ManualUpdate"));
        CRASH_UNLESS((il2cpp_utils::RunMethod<Il2CppObject*, false>(TrickSaber, update)));
    }

  private:
    inline static const std::unordered_set<std::string> ForbiddenComponents = {
        "::VRController", "::VRControllersValueSOOffsets"
    };
    int saberType;
    bool attachedForSpin = false;
    Il2CppObject* RealModel = nullptr;   // GameObject
    Il2CppObject* TrickModel = nullptr;  // GameObject
    Il2CppObject* RealSaber = nullptr;   // Saber
    Il2CppObject* TrickSaber = nullptr;  // Saber
    Il2CppObject* RealT = nullptr;       // Transform
    Il2CppObject* TrickT = nullptr;      // Transform
    Il2CppObject* UnattachedP = nullptr; // Transform
    Il2CppObject* AttachedP = nullptr;   // Transform
    inline static const MethodInfo* Transform_SetParent = nullptr;

    void _UpdateComponentsWithSaber(Il2CppObject* saber) {
        for (auto* type : tBurnTypes) {
            auto* components = CRASH_UNLESS(il2cpp_utils::RunMethod<Array<Il2CppObject*>*>(
                "UnityEngine", "Object", "FindObjectsOfType", type));
            for (il2cpp_array_size_t i = 0; i < components->Length(); i++) {
                auto* sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Il2CppObject*>*>(components->values[i], "_sabers"));
                sabers->values[saberType] = saber;
            }
        }

    }
};
