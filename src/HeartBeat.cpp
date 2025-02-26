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

DEFINE_TYPE(HeartBeat, HeartBeatObj);

#define GAMECORE_DEFAULT_TEXT "???\nbpm"

namespace HeartBeat{
    void HeartBeatObj::Start(){
        text = this->GetComponent<TMPro::TextMeshProUGUI*>();
        if(text == nullptr){
            getLogger().info("the text create failed.!");
        }else{
            // text->set_text("??? bpm");
            // text->set_color(getModConfig().HeartTextColor.GetValue());
            getLogger().info("the text has craeted!");
        }

        this->flash_remains = 0;
    }

    inline UnityEngine::Color GetFlashColor(ModConfig_t & conf){
        return HeartBeat::Recorder::isReplaying() ? conf.HeartReplayDataComeFlashColor.GetValue() : conf.HeartDataComeFlashColor.GetValue();
    }

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
            char buff[1024];
            
            if(false && getModConfig().DisplayEnergy.GetValue()){
                //energy not work, maybe something is missed in bluetooth protocol
                long long energy = HeartBeat::DataSource::getInstance()->GetEnergy();
                sprintf(buff, "% 3d bpm\n% 3lld KJ", data, energy);
            }else{
                sprintf(buff, "% 3d bpm", data);
            }
            
            text->set_text(buff);
            FlashColor();
        }
        
        if(flash_remains > 0){
            flash_remains -= UnityEngine::Time::get_deltaTime();
            if(flash_remains < 0){
                flash_remains = 0;
            }
            auto & conf = getModConfig();
            float total_time = conf.HeartDataComeFlashDuration.GetValue();
            float r = flash_remains / total_time;
            if(total_time > 0){
                text->set_color(UnityEngine::Color::Lerp(conf.HeartTextColor.GetValue(),  GetFlashColor(conf), 
                    r));
            }
        }
    }

    void HeartBeatObj::FlashColor(){
        auto & conf = getModConfig();
        flash_remains = conf.HeartDataComeFlashDuration.GetValue();
        if(flash_remains < 0.01){
            flash_remains = 0;
            return;
        }
        text->set_color(GetFlashColor(conf));
    }
};
