#pragma once

#include <dlfcn.h>
#include <unordered_map>
#include <unordered_set>
#include <UnityEngine/BoxCollider.hpp>

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "AllEnums.hpp"
#include "InputHandler.hpp"
#include "SaberTrickModel.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "UnityEngine/Transform.hpp"

// Conventions:
// tX means the type (usually System.Type i.e. Il2CppReflectionType) of X
// cX means the Il2CppClass of X
// xT means the .transform of unity object X


const UnityEngine::Vector3 Vector3_Zero = UnityEngine::Vector3(0.0f, 0.0f, 0.0f);
const UnityEngine::Vector3 Vector3_Right = UnityEngine::Vector3(1.0f, 0.0f, 0.0f);
const UnityEngine::Quaternion Quaternion_Identity = UnityEngine::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);


struct ValueTuple {
    UnityEngine::Vector3 item1;
    UnityEngine::Quaternion item2;
};


struct ButtonMapping {
    public:
		bool left;
		// static ButtonMapping LeftButtons;
		// static ButtonMapping RightButtons;
		std::unordered_map<int, std::unordered_set<std::unique_ptr<InputHandler>>> actionHandlers;

		void Update();

        ButtonMapping() {};
        ButtonMapping(bool isLeft) {
			left = isLeft;
			Update();
        };
};


class TrickManager {
    public:
        void LogEverything();
        bool doClashEffect = true;
        bool _isLeftSaber = false;
        GlobalNamespace::Saber* Saber;         // ::Saber
        GlobalNamespace::VRController* VRController;  // ::VRController
		TrickManager* other = nullptr;
		static void StaticClear();
		void Clear();
		void Start();
		void EndTricks();
		static void StaticPause();
		void PauseTricks();
		static void StaticResume();
    	void ResumeTricks();
		static void StaticFixedUpdate();
		void FixedUpdate();
        void Update();

	protected:
		TrickState _throwState;  // initialized in Start
		TrickState _spinState;

    private:
		void Start2();
		UnityEngine::Transform * FindBasicSaberTransform();
        ValueTuple GetTrackingPos();
        void CheckButtons();
        void ThrowStart();
        void ThrowUpdate();
        void ThrowReturn();
        void ThrowEnd();
        void InPlaceRotationStart();
		void InPlaceRotation(float power);
		void _InPlaceRotate(float amount);
		void InPlaceRotationReturn();
        void InPlaceRotationEnd();
		void TrickStart();
		void TrickEnd();
		void AddProbe(const UnityEngine::Vector3& vel, const UnityEngine::Vector3& ang);
        UnityEngine::Vector3 GetAverageVelocity();
        UnityEngine::Vector3 GetAverageAngularVelocity();
        GlobalNamespace::IVRPlatformHelper* _vrPlatformHelper;  			// ::VRPlatformHelper
        ButtonMapping _buttonMapping;
        UnityEngine::BoxCollider* _collider = nullptr;    		// BoxCollider
        UnityEngine::Vector3       _controllerPosition = Vector3_Zero;
        UnityEngine::Quaternion    _controllerRotation = Quaternion_Identity;
        UnityEngine::Vector3       _prevPos            = Vector3_Zero;
        UnityEngine::Vector3       _angularVelocity    = Vector3_Zero;
        UnityEngine::Quaternion    _prevRot            = Quaternion_Identity;
        float         _currentRotation;
        float         _saberSpeed         = 0.0f;
        float         _saberRotSpeed      = 0.0f;
		size_t _currentProbeIndex;
		std::vector<UnityEngine::Vector3> _velocityBuffer;
		std::vector<UnityEngine::Vector3> _angularVelocityBuffer;
		float _spinSpeed;
		float _finalSpinSpeed;
		SaberTrickModel* _saberTrickModel = nullptr;
		float _timeSinceStart = 0.0f;
		Il2CppString* _saberName = nullptr;
		Il2CppString* _basicSaberName = nullptr;  // only exists up until Start2
		UnityEngine::Transform* _saberT = nullptr;  // needed for effecient Start2 checking in Update
		// Vector3 _VRController_position_offset = Vector3_Zero;
        UnityEngine::Vector3 _throwReturnDirection = Vector3_Zero;
		// float _prevThrowReturnDistance;
		UnityEngine::Transform* _fakeTransform;  // will "replace" VRController's transform during trickCutting throws
};
