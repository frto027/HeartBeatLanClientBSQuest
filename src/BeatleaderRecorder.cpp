#include "BeatLeaderRecorder.hpp"
#include "HeartBeatDataSource.hpp"
#include "conditional-dependencies/shared/main.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <arpa/inet.h>
#include <sys/endian.h>
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "bs-utils/shared/utils.hpp"
#include "main.hpp"
#include "ModConfig.hpp"

namespace HeartBeat{
namespace Recorder{

bool needRecord = false;
bool recordStarted = false;
bool isPaused = false;

// the replay is not supported for this version
bool replayStarted = false;
int nextDataToReplay = 0;

//don't record too often
float_t lastRecordSongTime = -1000;
#define MIN_REDORD_TIME_INVERVAL 0.3f

struct RecordEntry{
    float timestamp;
    unsigned int heartrate;
};
std::vector<RecordEntry> recordData;
std::string heartDeviceName = HEART_DEV_NAME_UNK;

void RecordCallback(std::string name, int* length, void** data){
    recordStarted = false;

    static std::vector<uint8_t> datas;

    datas.clear();

    auto PushUInt32 = [](unsigned int x){
        x = htole32(x);
        uint8_t * d = (uint8_t*)&x;
        datas.push_back(d[0]);
        datas.push_back(d[1]);
        datas.push_back(d[2]);
        datas.push_back(d[3]);
    };
    auto PushFloat = [](float x){
        uint8_t * d = (uint8_t*)&x;
        datas.push_back(d[0]);
        datas.push_back(d[1]);
        datas.push_back(d[2]);
        datas.push_back(d[3]);
    };
    auto PushStr = [&](const char * str){
        size_t len = strlen(str);
        PushUInt32(len);
        for(int i=0;i<len;i++){
            datas.push_back(str[i]);
        }
    };
    
    size_t recordCount = recordData.size();

    getLogger().info("encoding {} heart record to replay.", recordCount);

    PushUInt32(1); // version

    PushUInt32(recordCount);
    for(int i=0;i<recordCount;i++){
        PushFloat(recordData[i].timestamp);
        PushUInt32(recordData[i].heartrate);
    }

    if(getModConfig().EnableRecord.GetValue() && getModConfig().RecordDevName.GetValue()){
        PushStr(heartDeviceName.c_str());
    }else{
        PushStr(HEART_DEV_NAME_HIDE);
    }
    
    *length = datas.size();
    *data = datas.data();

    recordData.clear();
}

GlobalNamespace::AudioTimeSyncController* audioTimeSyncController = NULL;
MAKE_HOOK_MATCH(ScoreControllerStart, &GlobalNamespace::ScoreController::Start, void, GlobalNamespace::ScoreController* self) {
    ScoreControllerStart(self);
    audioTimeSyncController = self->_audioTimeSyncController;
}

// copy from beatleader mod
inline bool UploadDisabledByReplay() {
    for (auto kv : bs_utils::Submission::getDisablingMods()) {
        if (kv.id == "Replay") {
            return true;
        }
    }
    return false;
}

MAKE_HOOK_MATCH(SinglePlayerInstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void, GlobalNamespace::GameplayCoreInstaller* self) {
    SinglePlayerInstallBindings(self);

    auto DisableRecord = [](){
        recordStarted = false;
        isPaused = false;
        recordData.clear();
    };

    if(dataSourceType == DS_RANDOM){
        DisableRecord();
        getLogger().info("random datasource will not enable record.");
        return;

    }

    if(UploadDisabledByReplay()){
        DisableRecord();
        getLogger().info("this is a replay, don't start record.");
        return;
    }

    if(!(getModConfig().EnableRecord.GetValue())){
        DisableRecord();
        getLogger().info("the player doesn't enable record, don't start record.");
        return;
    }

    getLogger().info("start record heart infos");
    recordData.clear();
    recordStarted = true;
    lastRecordSongTime = -1000;
    isPaused = false;
}

MAKE_HOOK_MATCH(LevelPause, &GlobalNamespace::PauseMenuManager::ShowMenu, void, GlobalNamespace::PauseMenuManager* self) {
    LevelPause(self);
    isPaused = true;
}

MAKE_HOOK_MATCH(LevelUnpause, &GlobalNamespace::PauseMenuManager::HandleResumeFromPauseAnimationDidFinish, void, GlobalNamespace::PauseMenuManager* self) {
    LevelUnpause(self);
    isPaused = false;
}

void Init(){
    auto AddReplayCustomDataProvider = CondDeps::FindUnsafe<void, std::string, std::function<void(std::string, int*, void**)> >("bl", "AddReplayCustomDataProvider");

    if(AddReplayCustomDataProvider.has_value()){
        getLogger().info("Beatleader is detected, enable record support");
        needRecord = true;
        AddReplayCustomDataProvider.value()("HeartBeatQuest", RecordCallback);
        INSTALL_HOOK(getLogger(), ScoreControllerStart);
        INSTALL_HOOK(getLogger(), SinglePlayerInstallBindings);
        INSTALL_HOOK(getLogger(), LevelPause);
        INSTALL_HOOK(getLogger(), LevelUnpause);
    }
}


void RecordDataIfNeeded(int heartrate){
    if(needRecord && audioTimeSyncController && recordStarted && !isPaused){
        float_t now = audioTimeSyncController->songTime;

        if(now >= lastRecordSongTime && now < lastRecordSongTime + MIN_REDORD_TIME_INVERVAL)
            return;
        lastRecordSongTime = now;
        // getLogger().info("recording {} {}", audioTimeSyncController->songTime, heartrate);
        recordData.emplace_back(audioTimeSyncController->songTime, heartrate);
    }
}

bool isReplaying(){
    return replayStarted;
}
bool ReplayGetData(int &heartrate){
    if(replayStarted && audioTimeSyncController && nextDataToReplay < recordData.size() && audioTimeSyncController->songTime >= recordData[nextDataToReplay].timestamp){
        heartrate = recordData[nextDataToReplay].heartrate;
        nextDataToReplay++;
        return true;
    }
    return false;
}

}
}