#include "HeartBeatDataSource.hpp"
#include "UnityEngine/Time.hpp"

#include "../shared/HeartBeatApi.h"
#include <cstddef>

namespace HeartBeat{
    namespace ApiInternal{
        int (*AlternateDataUpdater)(int *heartBeat) = nullptr;

        int data;
        int hasNewData = false;
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

        int GetData(int * heartbeat){
            if(heartbeat){
                *heartbeat = data;
            }
            return hasNewData;
        }
    }
}

extern "C" HeartBeatApi heartBeatApi = {
    .ApiVersion = 1,
    .Update = HeartBeat::ApiInternal::Update,
    .GetData = HeartBeat::ApiInternal::GetData,
    .SetAlternateDataUpdater = [](auto t){
        HeartBeat::ApiInternal::AlternateDataUpdater = t;
    },
    .__not_used2__ = {NULL},
};
