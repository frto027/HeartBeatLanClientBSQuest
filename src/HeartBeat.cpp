#include "ModConfig.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextMeshPro.hpp"
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

#include "paper/shared/logger.hpp"

#include "HeartBeatDataSource.hpp"
#include "HeartBeatApiInternal.hpp"

#include "BeatLeaderRecorder.hpp"

#include "UnityEngine/AssetBundle.hpp"
#include "stdio.h"
DEFINE_TYPE(HeartBeat, HeartBeatObj);

#define GAMECORE_DEFAULT_TEXT "???\nbpm"

namespace HeartBeat{
    void HeartBeatObj::Update(){
        if(this->gameObject->activeInHierarchy == false)
            return;
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
            int Maximum = 220 - getModConfig().MaxHeart.GetValue();
            float percent = ((float)data) / Maximum;


            char buff[256];
            sprintf(buff, "%d", data);
            for(auto text : loadedComponents.heartrateTexts)
                text->set_text(buff);
            if(loadedComponents.animator){
                //loadedComponents.animator->SetInteger("age", 25);
                loadedComponents.animator->SetInteger("heartrate", data);
                loadedComponents.animator->SetFloat("heartpercent", percent);
                loadedComponents.animator->SetTrigger("datacome");
            }
        }
    }
};

#define ASSET_UI_PATH "/sdcard/ModData/com.beatgames.beatsaber/Mods/HeartBeatQuest/UI/"

bool endsWith(const char * a, const char *b){
    size_t lena = strlen(a);
    size_t lenb = strlen(b);
    if(lena < lenb)
        return false;
    if(strcmp(a + lenb, b) == 0)
        return true;
    return false;
}

namespace HeartBeat{
    AssetBundleManager assetBundleMgr;
    
    
    void AssetBundleManager::Init(){
        if(initialized)return;
        initialized = true;

        auto LoadAssetBundle = [this](UnityW<UnityEngine::AssetBundle> bundle){
            for(auto name : bundle->GetAllAssetNames()){
                auto gameObject = bundle->LoadAsset<UnityEngine::GameObject*>(name);
                if(gameObject){
                    auto info = gameObject->get_transform()->Find("info");
                    if(!info)
                        continue;
                    
                    AssetUI assetUI = {};

                    for(int i=0;i<info->get_childCount();i++){
                        auto name = info->GetChild(i)->get_name();
                        if(name){
                            auto col = name->IndexOf(':');
                            if(col > 0){
                                auto key = name->Substring(0, col);
                                auto val = name->Substring(col+1);
                                assetUI.infos[std::string(key)] = std::string(val);
                            }
                        }
                    }
                    
                    std::string name = "NoName";
                    if(assetUI.infos.contains("name"))
                        name = assetUI.infos["name"];
                    if(loadedBundles.contains(name)){
                        size_t malloc_size = name.size() + 10;
                        char * buff = (char*)malloc(malloc_size);
                        for(int i=0;i<100;i++){
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
                    assetUI.prefab = gameObject;
                    getLogger().info("Loaded UI {}", name);
                    loadedBundles.insert({name, assetUI});
                }
            }
        };

        #include "DefaultUI.inl"

        auto AssetBundle_LoadFromMemory = (function_ptr_t<UnityEngine::AssetBundle*,ArrayW<uint8_t>, uint32_t>)CRASH_UNLESS(il2cpp_functions::resolve_icall("UnityEngine.AssetBundle::LoadFromMemory"));
        ArrayW<uint8_t> data(sizeof(default_ui));
        memmove(data->begin(), default_ui, sizeof(default_ui));
        try{
            LoadAssetBundle(AssetBundle_LoadFromMemory(data, 0));
        }catch(...){
            getLogger().error("Can't load default ui");
        }

        if(std::filesystem::is_directory(ASSET_UI_PATH)){
            std::filesystem::directory_iterator it(ASSET_UI_PATH);
            if(it->is_regular_file() && endsWith(it->path().c_str(), ".bundle")){
                try{
                    LoadAssetBundle(UnityEngine::AssetBundle::LoadFromFile(it->path().c_str()));
                }catch(...){
                    getLogger().error("Can't load asset file {}", it->path().c_str());
                }
            }
        }

    }

    bool AssetBundleManager::Instantiate(std::string name, UnityEngine::Transform * parent, AssetBundleInstinateInformation & result){
        if(!loadedBundles.contains(name))
            return false;
        auto & assetUI = loadedBundles[name];
        auto gameobject = UnityEngine::Object::Instantiate(UnityW<UnityEngine::GameObject>(assetUI.prefab.ptr()) /* This gameobject will not be GC if we use it, be careful. */);
        auto FindAll = [&](UnityEngine::Transform * transform){
            if(transform->get_tag()->Equals("heartrate")){
                auto tm = transform->GetComponent<TMPro::TMP_Text *>();
                if(tm){
                    result.heartrateTexts.push_back(tm);
                }
            }
        };

        FindAll(gameobject->get_transform());
        result.animator = gameobject->GetComponent<UnityEngine::Animator*>();
        result.gameObject = gameobject;
        return true;
    }
}