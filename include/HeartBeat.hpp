#pragma once

#include "HMUI/CurvedTextMeshPro.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "custom-types/shared/macros.hpp"
#include "TMPro/TextMeshPro.hpp"
#include <map>
#include "UnityEngine/Animator.hpp"


namespace HeartBeat{

extern const char *ui_features[];

struct AssetUI{
    std::optional<std::string> filePath;
    std::string AssetPath;
    std::map<std::string, std::string> infos;
    std::set<std::string> supported_features;
    std::set<std::string> unsupported_features;
};

struct AssetBundleInstinateInformation{
    std::vector<TMPro::TMP_Text*> heartrateTexts;
    std::vector<UnityEngine::Animator *> animators;
    UnityEngine::GameObject * gameObject;
};
struct AssetBundleManager{
    bool initialized = false;

    std::map<std::string, AssetUI> loadedBundles;
    void Init();

    bool Instantiate(std::string name, UnityEngine::Transform * parent, AssetBundleInstinateInformation & result);

    static std::set<std::string> GetFeatures(std::string feature);
};

extern AssetBundleManager assetBundleMgr;

}


// parameters are (namespace, class name, parent class, contents)
#if defined(GAME_VER_1_28_0) || defined(GAME_VER_1_35_0) || defined(GAME_VER_1_37_0)
DECLARE_CLASS_CODEGEN(HeartBeat, HeartBeatObj, UnityEngine::MonoBehaviour,
#else
DECLARE_CLASS_CODEGEN(HeartBeat, HeartBeatObj, UnityEngine::MonoBehaviour){
#endif
    // DECLARE_INSTANCE_METHOD creates methods
    DECLARE_INSTANCE_METHOD(void, Start);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);
    DECLARE_INSTANCE_METHOD(void, Update);
public:
    AssetBundleInstinateInformation loadedComponents;
    HMUI::CurvedTextMeshPro * serverMessageDisplayer = nullptr;
#if defined(GAME_VER_1_28_0) || defined(GAME_VER_1_35_0) || defined(GAME_VER_1_37_0)
);
#else
};
#endif


