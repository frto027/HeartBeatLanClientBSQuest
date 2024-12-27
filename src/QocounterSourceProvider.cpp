
#include "UnityEngine/UI/Graphic.hpp"
#include "bsml/shared/BSML-Lite/Creation/Text.hpp"
#include "scotland2/shared/loader.hpp"
#include "UnityEngine/GameObject.hpp"
#include "rapidjson-macros/shared/macros.hpp"
#include <cstddef>
#include <dlfcn.h>
#include <string>
#include "main.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RenderMode.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "ModConfig.hpp"
#include "HeartBeat.hpp"

namespace Qounters::Types {
    template <class T>
    using SourceFn = std::function<T(UnparsedJSON)>;
    using SourceUIFn = std::function<void(UnityEngine::GameObject*, UnparsedJSON)>;
    using PremadeFn = std::function<UnityEngine::UI::Graphic*(UnityEngine::GameObject*, UnparsedJSON)>;
    using PremadeUIFn = std::function<void(UnityEngine::GameObject*, UnparsedJSON)>;
    using PremadeUpdateFn = std::function<void(UnityEngine::UI::Graphic*, UnparsedJSON)>;
    using TemplateUIFn = std::function<void(UnityEngine::GameObject*)>;
}
namespace Qounters::API {
    // don't call this function, it's only used for type and will finally will not link.
    void (*RegisterPremade)(
        std::string mod, std::string name, Types::PremadeFn creation, Types::PremadeUIFn uiFunction, Types::PremadeUpdateFn update
    ) = nullptr;
}

int (*RegisterCustomEvent)(std::string mod, int event) = nullptr;
void (*Events_Broadcast)(std::string mod, int event) = nullptr;

/////////////////////////////////////////////////////

#define QOOUNTER_EVENT_HEART_DATA_UPDATE 1

namespace HeartBeat{
    UnityEngine::UI::Graphic* createFn(UnityEngine::GameObject* obj, UnparsedJSON){
        auto text = BSML::Lite::CreateText(obj->get_transform(), "");
        auto rect = text->get_rectTransform();
        rect->SetParent(obj->transform, false);
        rect->anchoredPosition = {0.5, 0.5};
        rect->sizeDelta = {180, 20};

        text->color = getModConfig().HeartTextColor.GetValue();
        text->fontSize = 10;
        text->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);
        text->get_gameObject()->AddComponent<HeartBeat::HeartBeatObj*>()->InitComponent()->isQounterComponent = 1;

        return text;
    }

    // the following code will not work until qounters++, because qounters++ has not expose the enough api
    // int _heart =0;
    void InformHeartForQounters(int heart){
        // if(!Events_Broadcast) return;
        // _heart = heart;
        // Events_Broadcast("HeartBeatQuest", QOOUNTER_EVENT_HEART_DATA_UPDATE);
    }
    // std::string StringSource(UnparsedJSON){
    //     char buff[128];
    //     sprintf(buff, "%d bpm", _heart);
    //     return buff;
    // }
    // UnityEngine::Color ColorSource(UnparsedJSON json){
    //     return UnityEngine::Color::get_cyan();
    // }


    void InitQocunterSource(){
        bool anythingDone = false;
        for(auto & modData : modloader::get_loaded()){
            if(modData.info.id == "qounters++"){
                // Qounters::API::RegisterPremade("HeartBeatQuest", "heart rate", createFn);
                Qounters::API::RegisterPremade = (decltype(Qounters::API::RegisterPremade))dlsym(modData.handle, "_ZN8Qounters3API15RegisterPremadeENSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEES7_NS1_8functionIFPN11UnityEngine2UI7GraphicEPNS9_10GameObjectE12UnparsedJSONEEENS8_IFvSE_SF_EEENS8_IFvSC_SF_EEE");
                if(Qounters::API::RegisterPremade == NULL){
                    getLogger().info("qounters++ has been found, but RegisterPremade function can't be found.");
                }else{
                    anythingDone = true;
                    getLogger().info("qounters++ has been found, call RegisterPremade to add component");
                    Qounters::API::RegisterPremade("HeartBeatQuest", "heart rate", createFn, nullptr, nullptr);
                }

                RegisterCustomEvent = (decltype(RegisterCustomEvent))dlsym(modData.handle, "_Z19RegisterCustomEventNSt6__ndk112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEi");
                if(RegisterCustomEvent){
                    getLogger().info("Register events");
                    RegisterCustomEvent("HeartBeatQuest", QOOUNTER_EVENT_HEART_DATA_UPDATE);
                }

                Events_Broadcast = (decltype(Events_Broadcast))dlsym(modData.handle, "_ZN8Qounters6Events9BroadcastENSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEi");
            }
        }
        if(!anythingDone){
            getLogger().info("qounters++ not found, skip.");
        }
    }
}