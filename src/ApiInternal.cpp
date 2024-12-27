#include "HeartBeatDataSource.hpp"
#include "UnityEngine/Time.hpp"

#include "../shared/HeartBeatApi.h"

namespace HeartBeat{
    namespace ApiInternal{
        bool (*AlternateDataUpdater)(int *heartBeat) = nullptr;

        int data;
        bool hasNewData = false;
        void Update(){
            static int lastUpdateFrame = 0;
            int curFrame = UnityEngine::Time::get_frameCount();
            if(curFrame != lastUpdateFrame){
                lastUpdateFrame = curFrame;

                if(AlternateDataUpdater){
                    hasNewData = AlternateDataUpdater(&data);
                }else{
                    auto instance = HeartBeat::DataSource::getInstance();
                    instance->Update();
                    hasNewData = instance->GetData(data);
                }
            }
        }

        bool GetData(int * heartbeat){
            if(heartbeat){
                *heartbeat = data;
            }
            return hasNewData;
        }
    }
}

namespace HeartBeatApi{
    class Api_impl : virtual public Api{
        const char * Version(){
            return VERSION;
        }
        void Update(){
            HeartBeat::ApiInternal::Update();
        }
        bool GetData(int * heartbeat){
            return HeartBeat::ApiInternal::GetData(heartbeat);
        }
        void SetAlternateDataUpdater(bool (*Updater)(int* heart_output)){
            HeartBeat::ApiInternal::AlternateDataUpdater = Updater;
        }
    };

    extern Api_impl api;
    Api_impl api;
}

