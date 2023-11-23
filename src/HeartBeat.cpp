#include "main.hpp"
#include "HeartBeat.hpp"
#include "codegen/include/UnityEngine/Transform.hpp"
#include "codegen/include/UnityEngine/RectTransform.hpp"
#include "codegen/include/UnityEngine/GameObject.hpp"
#include "codegen/include/UnityEngine/MonoBehaviour.hpp"

#include "HeartBeatDataSource.hpp"

DEFINE_TYPE(HeartBeat, HeartBeatObj);

namespace HeartBeat{
    void HeartBeatObj::Start(){
        text = this->get_gameObject()->AddComponent<TMPro::TextMeshPro*>();
        if(text == nullptr){
            getLogger().info("the text create failed.!");
        }else{
            auto rectTransform = text->get_rectTransform();
            rectTransform->set_position({0,2.5,3.5});
            text->set_alignment(TMPro::TextAlignmentOptions::Center);
            text->set_fontSize(3);
            text->set_text("heart heart heart");
        }
    }
    void HeartBeatObj::Update(){
        auto instance = HeartBeat::DataSource::getInstance();
        instance->Update();

        int data;
        if(HeartBeat::DataSource::getInstance()->GetData(data)){
            char buff[1024];
            sprintf(buff, "heart % 3d", data);
            text->set_text(buff);
        }
    }
};
