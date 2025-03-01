#pragma once

#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "custom-types/shared/macros.hpp"
#include "TMPro/TextMeshPro.hpp"
#include <map>
#include "UnityEngine/Animator.hpp"


namespace HeartBeat{

struct AssetUI{
    std::map<std::string, std::string> infos;
    SafePtrUnity<UnityEngine::GameObject> prefab = {nullptr};
};

struct AssetBundleInstinateInformation{
    std::vector<TMPro::TMP_Text*> heartrateTexts;
    UnityEngine::Animator * animator;
    UnityEngine::GameObject * gameObject;
};
struct AssetBundleManager{
    bool initialized = false;

    std::map<std::string, AssetUI> loadedBundles;
    void Init();

    bool Instantiate(std::string name, UnityEngine::Transform * parent, AssetBundleInstinateInformation & result);
};

extern AssetBundleManager assetBundleMgr;

}


// parameters are (namespace, class name, parent class, contents)
DECLARE_CLASS_CODEGEN(HeartBeat, HeartBeatObj, UnityEngine::MonoBehaviour,
    // DECLARE_INSTANCE_METHOD creates methods
    DECLARE_INSTANCE_METHOD(void, Update);
public:
    AssetBundleInstinateInformation loadedComponents;
);


