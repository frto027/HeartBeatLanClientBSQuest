#include "ModConfig.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextMeshPro.hpp"
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

    enum HeartRateZone{
        Zone0, // None, lower than 50%
        Zone1, // 50% - 60%
        Zone2, // 60% - 70%
        Zone3, // 70% - 80%
        Zone4, // 80% - 90%
        Zone5  // 90% - 100%
    };

    HeartRateZone GetZone(int heart){
        int Maximum = 220 - getModConfig().Age.GetValue();
        int area = heart * 10 / Maximum;
        if(area >= 9)
            return Zone5;
        switch(area){
            case 8:return Zone4;
            case 7:return Zone3;
            case 6:return Zone2;
            case 5:return Zone1;
            default:return Zone0;
        }
    }

    void HeartBeatObj::SetColor(float percent, int dataHint){
        //percent = 0 : Normal
        //percent = 1 : Data Come and flash
        auto & conf = getModConfig();

        UnityEngine::Color TextColor,FlashColor;
        // auto Zone = GetZone(dataHint);
        // switch(Zone){
        // #define CASE(Zone) case Zone : TextColor = conf.HeartTextColor##Zone.GetValue(); break;
        //     CASE(Zone0)
        //     CASE(Zone1)
        //     CASE(Zone2)
        //     CASE(Zone3)
        //     CASE(Zone4)
        //     CASE(Zone5)
        // #undef CASE
        // }

        TextColor = conf.HeartTextColor.GetValue();
        FlashColor = conf.HeartDataComeFlashColor.GetValue();

        text->set_color(UnityEngine::Color::Lerp(TextColor, FlashColor, percent));
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
            FlashColor(data);
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
                SetColor(r, data);
            }
        }
    }

    void HeartBeatObj::FlashColor(int dataHint){
        auto & conf = getModConfig();
        flash_remains = conf.HeartDataComeFlashDuration.GetValue();
        if(flash_remains < 0.01){
            flash_remains = 0;
            return;
        }
        SetColor(1, dataHint);
    }
};
