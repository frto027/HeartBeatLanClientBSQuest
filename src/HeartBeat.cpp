#include "ModConfig.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextMeshPro.hpp"
#include "UnityEngine/Animator.hpp"
#include "UnityEngine/Color.hpp"
#include "bsml/shared/BSML-Lite/Creation/Text.hpp"
#include "main.hpp"
#include "HeartBeat.hpp"
#include "HeartBeatSetthings.hpp"

#include "UnityEngine/Transform.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Quaternion.hpp"

#include "multi_version_compat.hpp"

#include "HeartBeatDataSource.hpp"
#include "HeartBeatApiInternal.hpp"

#include "BeatLeaderRecorder.hpp"

#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/Resources.hpp"
#include "bsml/shared/Helpers/getters.hpp"

#include "stdio.h"
#include <cstddef>
#include <mutex>
DEFINE_TYPE(HeartBeat, HeartBeatObj);

const char *HeartBeat::ui_features[] = {
    NULL
};

namespace HeartBeat{
    void HeartBeatObj::Start(){
        if(HeartBeat::dataSourceType == HeartBeat::DS_HypeRate){
            HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatHypeRateDataSource>()->needConnection = true;
        }    
    }

    void HeartBeatObj::OnDestroy(){
        getLogger().info("Destroy, we don't need heart in the future");
        if(HeartBeat::dataSourceType == HeartBeat::DS_HypeRate){
            HeartBeat::DataSource::getInstance<HeartBeat::HeartBeatHypeRateDataSource>()->needConnection = false;
        }    

        // we will disable replay here (when the UI inside the game scene is destroyed), because I don't want hook a scene unload function.
        HeartBeat::Recorder::replayStarted = false;

    }
    void HeartBeatObj::Update(){
        if(this->gameObject->activeInHierarchy == false)
            return;
        if(this->serverMessageDisplayer){
            if(dataSourceType == DS_HypeRate){
                std::string message;
                bool has_message = false;
                auto * instance = DataSource::getInstance<HeartBeatHypeRateDataSource>();
                if(instance->has_message_from_server){
                    std::lock_guard<std::mutex> lock(instance->message_from_server_mutex);
                    if(instance->has_message_from_server){
                        message = instance->message_from_server;
                        has_message = true;
                    }
                }
                if(has_message)
                this->serverMessageDisplayer->set_text(message);
            }
        }
        {
            static int slow_down = 0;
            if(slow_down++ > 20){
                slow_down = 0;
                SetthingUI::UpdateSetthingsUI();
            }
        }
        HeartBeat::ApiInternal::Update();

        int data;
        if(HeartBeat::ApiInternal::GetData(&data)){
            int Maximum = getModConfig().MaxHeart.GetValue();
            float percent = ((float)data) / Maximum;


            char buff[256];
            sprintf(buff, "%d", data);
            for(auto text : loadedComponents.heartrateTexts)
                text->set_text(buff);
            for(auto anmt : loadedComponents.animators){
                //loadedComponents.animator->SetInteger("age", 25);
                anmt->SetInteger("heartrate", data);
                anmt->SetFloat("heartpercent", percent);
                anmt->SetTrigger("datacome");
                anmt->SetBool("replaying", HeartBeat::Recorder::isReplaying());
            }
        }
    }
};

#define ASSET_UI_PATH "/sdcard/ModData/com.beatgames.beatsaber/Mods/HeartBeatQuest/UI/"

namespace HeartBeat{
    AssetBundleManager assetBundleMgr;
    
    #include "DefaultUI.inl"

    void FixPrefab(UnityEngine::Transform * transform){
        auto tm = transform->GetComponent<TMPro::TMP_Text *>();
        if(tm){
            tm->set_font(BSML::Helpers::GetMainTextFont());
            tm->set_fontSharedMaterial(BSML::Helpers::GetMainUIFontMaterial());
        }
        for(int i=0;i<transform->get_childCount();i++){
            FixPrefab(transform->GetChild(i).ptr());
        }

    }

    void AssetBundleManager::Init(){
        if(initialized)return;
        initialized = true;

        auto LoadAssetBundle = [this](UnityEngine::AssetBundle* bundle, std::optional<std::string> filepath){
            auto assetNamesUnity = bundle->GetAllAssetNames();
            std::vector<std::string> assetPaths = {assetNamesUnity->begin(), assetNamesUnity->end()};
            for(auto assetPath : assetPaths){
                getLogger().info("Start load {}", assetPath);
                SafePtrUnity<UnityEngine::GameObject> gameObject = bundle->LoadAsset<UnityEngine::GameObject*>(assetPath);
                if(gameObject){
                    auto info = gameObject->get_transform()->Find("info");
                    if(!info)
                        continue;
                    
                    std::map<std::string, std::string> infos = {};

                    for(int i=0;i<info->get_childCount();i++){
                        auto name = info->GetChild(i)->get_name();
                        if(name){
                            auto col = name->IndexOf(':');
                            if(col > 0){
                                auto key = name->Substring(0, col);
                                auto val = name->Substring(col+1);
                                std::string key_str = key;
                                auto old_val_it = infos.find(key_str);
                                if(old_val_it == infos.end())
                                    infos[key_str] = std::string(val);
                                else
                                    infos[key_str] = old_val_it->second + "," + std::string(val);
                            }
                        }
                    }
                    
                    std::string name = "NoName";
                    if(infos.contains("name"))
                        name = infos["name"];
                    if(loadedBundles.contains(name)){
                        size_t malloc_size = name.size() + 10;
                        char * buff = (char*)malloc(malloc_size);
                        for(int i=2;i<100;i++){
                            sprintf(buff, "%s %d", name.c_str(), i);
                            if(!loadedBundles.contains(buff))
                                break;
                        }
                        name = buff;
                        free(buff);
                    }
                    if(loadedBundles.contains(name)){
                        continue;
                    }

                    std::set<std::string> unsupported_features;
                    std::set<std::string> supported_features;
                    if(infos.contains("feature")){
                        unsupported_features = GetFeatures(infos["feature"]);
                    }
                    
                    for(const char ** feature = ui_features; *feature; feature++){
                        auto it = unsupported_features.find(*feature);
                        if(it != unsupported_features.end()){
                            supported_features.insert(*feature);
                            unsupported_features.erase(it);
                        }
                    }
                    getLogger().info("Loaded UI, asset name: '{}'", name);
                    if(unsupported_features.size() > 0){
                        getLogger().info("  {} features are unsupported.", unsupported_features.size());
                        for(auto & feature : unsupported_features){
                            getLogger().info("    feature unsupported: {}", feature);
                        }
                    }
                    loadedBundles.insert({name, {filepath, assetPath, std::move(infos), std::move(supported_features), std::move(unsupported_features)}});
                }
            }
            getLogger().info("bundle load over");
        };

        auto AssetBundle_LoadFromMemory = (function_ptr_t<UnityEngine::AssetBundle*,ArrayW<uint8_t>, uint32_t>)CRASH_UNLESS(il2cpp_functions::resolve_icall("UnityEngine.AssetBundle::LoadFromMemory_Internal"));
        ArrayW<uint8_t> data(sizeof(default_ui));
        memcpy(data->begin(), default_ui, sizeof(default_ui));
        try{
            auto bundle = AssetBundle_LoadFromMemory(data, 0);
            LoadAssetBundle(bundle, {});
            getLogger().info("Unload bundle {}", (void*)bundle);
            bundle->Unload(true);
            getLogger().info("done");
        }catch(...){
            getLogger().error("Can't load default ui");
        }
        getLogger().info("Start loading bundles from directory");
        if(std::filesystem::is_directory(ASSET_UI_PATH)){

            for(auto& entry :std::filesystem::directory_iterator(ASSET_UI_PATH)){
                getLogger().info("Handling {}", entry.path().c_str());
                if(entry.is_regular_file() && entry.path().has_extension() && entry.path().extension() == ".bundle"){
                    try{
                        auto bundle = UnityEngine::AssetBundle::LoadFromFile(entry.path().c_str());
                        LoadAssetBundle(bundle, entry.path());
                        bundle->Unload(true);
                    }catch(...){
                        getLogger().error("Can't load asset file {}", entry.path().c_str());
                    }
                }
            }
        }
        getLogger().info("directory load over");
    }

    void HandleTransformsInBundle(AssetBundleInstinateInformation & result, UnityEngine::Transform * transform){
        {
            auto tm = transform->GetComponent<TMPro::TMP_Text *>();
            if(tm){
                if(transform->get_name()->Equals("auto:heartrate")){
                    result.heartrateTexts.push_back(tm);
                }
            }
            auto anmt = transform->GetComponent<UnityEngine::Animator*>();
            if(anmt){
                result.animators.push_back(anmt);
            }
        }
        for(int i=0;i<transform->get_childCount();i++){
            HandleTransformsInBundle(result, transform->GetChild(i).ptr());
        }
    }

    bool AssetBundleManager::Instantiate(std::string name, UnityEngine::Transform * parent, AssetBundleInstinateInformation & result){
        if(!loadedBundles.contains(name))
            return false;
        auto & assetUI = loadedBundles[name];

        UnityEngine::AssetBundle * bundle = nullptr;
        {
            if(assetUI.filePath.has_value()){
                try{
                    bundle = UnityEngine::AssetBundle::LoadFromFile(assetUI.filePath.value());

                }catch(...){
                    getLogger().error("Can't load asset bundle {}", assetUI.filePath.value());
                }
            }else{
                ArrayW<uint8_t> data(sizeof(default_ui));
                memcpy(data->begin(), default_ui, sizeof(default_ui));
                try{
                    static std::optional<function_ptr_t<UnityEngine::AssetBundle*,ArrayW<uint8_t>, uint32_t>> AssetBundle_LoadFromMemory = {};
                    if(!AssetBundle_LoadFromMemory.has_value())
                        AssetBundle_LoadFromMemory = (function_ptr_t<UnityEngine::AssetBundle*,ArrayW<uint8_t>, uint32_t>)CRASH_UNLESS(il2cpp_functions::resolve_icall("UnityEngine.AssetBundle::LoadFromMemory_Internal"));

                    bundle = AssetBundle_LoadFromMemory.value()(data, 0);
                }catch(...){
                    getLogger().error("Can't load default ui");
                }
            }
        }

        if(bundle == nullptr){
            getLogger().error("UI AssetBundle load failed");
            return false;
        }

        UnityEngine::GameObject * prefab = bundle->LoadAsset<UnityEngine::GameObject *>(assetUI.AssetPath);
        if(prefab == nullptr){
            getLogger().error("Can't load prefab {}", assetUI.AssetPath);
            bundle->Unload(true);
            return false;
        }

        FixPrefab(prefab->get_transform());
        auto gameobject = UnityEngine::GameObject::Instantiate(prefab, parent);
        getLogger().info("InstinateDone");
        bundle->Unload(false);
        HandleTransformsInBundle(result, gameobject->get_transform());
        result.gameObject = gameobject;
        return true;
    }

    std::set<std::string> AssetBundleManager::GetFeatures(std::string feature){
        std::set<std::string> features;

        size_t before = 0;
        while(before < feature.size()){
            size_t next = feature.find(',', before);
            if(next == before){
                before = next + 1;
                continue;
            }
            if(next == std::string::npos){
                features.insert(feature.substr(before));
                break;
            }else{
                features.insert(feature.substr(before, next - before));
                before = next + 1;
            }
        }
        return std::move(features);
    }
}