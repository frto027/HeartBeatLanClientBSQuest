#include "main.hpp"
#include "HeartBeat.hpp"
#include "codegen/include/UnityEngine/Transform.hpp"
#include "codegen/include/UnityEngine/RectTransform.hpp"
#include "codegen/include/UnityEngine/GameObject.hpp"
#include "codegen/include/UnityEngine/MonoBehaviour.hpp"

#include "HeartBeatDataSource.hpp"

DEFINE_TYPE(HeartBeat, HeartBeatObj);

#define GAMECORE_DEFAULT_TEXT "???\nbpm"
#define MAINMENU_DEFAULT_TEXT "??? bpm"

namespace HeartBeat{
    void HeartBeatObj::Start(){
        heartbeatObj = this;
        text = this->get_gameObject()->AddComponent<TMPro::TextMeshPro*>();
        if(text == nullptr){
            getLogger().info("the text create failed.!");
        }else{
            auto rectTransform = text->get_rectTransform();
            text->set_alignment(TMPro::TextAlignmentOptions::Center);
            text->set_text(MAINMENU_DEFAULT_TEXT);
            text->set_lineSpacing(getModConfig().HeartLineSpaceDelta.GetValue());
            text->set_color(getModConfig().HeartTextColor.GetValue());
            this->SetStatus(HEARTBEAT_STATUS_MAINMENU);
        }
    }
    void HeartBeatObj::Update(){
        {
            static int slow_down = 0;
            if(slow_down++ > 20){
                slow_down = 0;
            ModUI::UpdateSetthingsUI();
            }
        }
        if(this->status == HEARTBEAT_STATUS_HIDE)
            return;
        auto instance = HeartBeat::DataSource::getInstance();
        instance->Update();

        int data;
        if(HeartBeat::DataSource::getInstance()->GetData(data)){
            char buff[1024];
            if(status == HEARTBEAT_STATUS_GAMECORE){
                sprintf(buff, "%d\nbpm", data);
            }else{
                sprintf(buff, "% 3d bpm", data);
            }
            text->set_text(buff);
        }
    }

    void HeartBeatObj::SetStatus(int status){
        this->status = status;
        switch (status)
        {
        case HEARTBEAT_STATUS_GAMECORE:
            GoToGameCorePos();
            //this->text->get_rectTransform()->set_position({3.1,0.2,6.8});
            text->set_fontSize(3);
            this->text->get_rectTransform()->set_rotation(UnityEngine::Quaternion::AngleAxis(getModConfig().HeartGameCoreRot.GetValue(), UnityEngine::Vector3::get_up()));
            this->text->set_text(GAMECORE_DEFAULT_TEXT);
            break;
        case HEARTBEAT_STATUS_MAINMENU:
            GoToMainMenuPos();
            //this->text->get_rectTransform()->set_position({-2.3,2.8,4});
            text->set_fontSize(2);
            this->text->get_rectTransform()->set_rotation(UnityEngine::Quaternion::AngleAxis(getModConfig().HeartMainMenuRot.GetValue(), UnityEngine::Vector3::get_up()));
            this->text->set_text(MAINMENU_DEFAULT_TEXT);
            break;
        case HEARTBEAT_STATUS_HIDE:
            this->text->set_text("");
            break;
        }
    }

    void HeartBeatObj::GoToGameCorePos(){
        this->text->get_rectTransform()->set_position(getModConfig().HeartGameCorePos.GetValue());
    }
    void HeartBeatObj::GoToMainMenuPos(){
        this->text->get_rectTransform()->set_position(getModConfig().HeartMainMenuPos.GetValue());
    }
};
