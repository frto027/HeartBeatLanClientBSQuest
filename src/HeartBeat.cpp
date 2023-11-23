#include "main.hpp"
#include "HeartBeat.hpp"
#include "codegen/include/UnityEngine/Transform.hpp"
#include "codegen/include/UnityEngine/RectTransform.hpp"
#include "codegen/include/UnityEngine/GameObject.hpp"
#include "codegen/include/UnityEngine/MonoBehaviour.hpp"

#include "HeartBeatDataSource.hpp"

DEFINE_TYPE(HeartBeat, HeartBeatObj);

namespace HeartBeat{
    #define DEFAULT_HEART_TEXT "heart ???"
    void HeartBeatObj::Start(){
        heartbeatObj = this;
        text = this->get_gameObject()->AddComponent<TMPro::TextMeshPro*>();
        if(text == nullptr){
            getLogger().info("the text create failed.!");
        }else{
            auto rectTransform = text->get_rectTransform();
            text->set_alignment(TMPro::TextAlignmentOptions::Center);
            text->set_text(DEFAULT_HEART_TEXT);
            text->set_lineSpacing(-35);
            this->SetStatus(HEARTBEAT_STATUS_MAINMENU);
        }
    }
    void HeartBeatObj::Update(){
        if(this->status == HEARTBEAT_STATUS_HIDE)
            return;
        auto instance = HeartBeat::DataSource::getInstance();
        instance->Update();

        int data;
        if(HeartBeat::DataSource::getInstance()->GetData(data)){
            char buff[1024];
            if(status == HEARTBEAT_STATUS_GAMECORE){
                sprintf(buff, "heart\n% 3d", data);
            }else{
                sprintf(buff, "heart % 3d", data);
            }
            text->set_text(buff);
        }
    }

    void HeartBeatObj::SetStatus(int status){
        this->status = status;
        switch (status)
        {
        case HEARTBEAT_STATUS_GAMECORE:
            this->text->get_rectTransform()->set_position({3.1,0.2,6.8});
            text->set_fontSize(3);
            this->text->get_rectTransform()->set_rotation(UnityEngine::Quaternion::AngleAxis(0, UnityEngine::Vector3::get_up()));
            this->text->set_text(DEFAULT_HEART_TEXT);
            break;
        case HEARTBEAT_STATUS_MAINMENU:
            // this->text->get_rectTransform()->set_position({0,3,4});
            this->text->get_rectTransform()->set_position({-2.3,2.8,4});
            text->set_fontSize(2);
            this->text->get_rectTransform()->set_rotation(UnityEngine::Quaternion::AngleAxis(-30, UnityEngine::Vector3::get_up()));
            this->text->set_text(DEFAULT_HEART_TEXT);
            break;
        case HEARTBEAT_STATUS_HIDE:
            this->text->set_text("");
            break;
        }
    }
};
