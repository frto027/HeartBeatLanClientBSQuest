#pragma once

#include "UnityEngine/MonoBehaviour.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "custom-types/shared/macros.hpp"
#include "TMPro/TextMeshPro.hpp"

// parameters are (namespace, class name, parent class, contents)
#define HEARTBEAT_STATUS_MAINMENU 1
#define HEARTBEAT_STATUS_GAMECORE 2
#define HEARTBEAT_STATUS_HIDE 3
DECLARE_CLASS_CODEGEN(HeartBeat, HeartBeatObj, UnityEngine::MonoBehaviour,
    // DECLARE_INSTANCE_METHOD creates methods
    DECLARE_INSTANCE_METHOD(void, Start);
    DECLARE_INSTANCE_METHOD(void, Update);

    DECLARE_INSTANCE_METHOD(void, SetStatus,int status);
    // DECLARE_INSTANCE_FIELD creates fields
    //DECLARE_INSTANCE_FIELD(int, counts);
public:
    TMPro::TextMeshPro* text;
    int status;

    float flash_remains;
    void GoToGameCorePos();
    void GoToMainMenuPos();
    void FlashColor();
);