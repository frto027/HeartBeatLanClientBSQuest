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
#include "main.hpp"
#include "ModConfig.hpp"
#include "metacore/shared/game.hpp"

#include <time.h>

inline double now_ms(void) {

    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6;

}
namespace HeartBeat{
namespace Recorder{

bool needRecord = false;
bool recordStarted = false;
bool isPaused = false;

bool needReplay = false;
// the replay is not supported for this version
bool replayStarted = false;
int currentDataToReplay = -1;

//don't record too often
float_t lastRecordSongTime = -1000;
#define MIN_REDORD_TIME_INVERVAL 0.3f

std::optional<bool(*)(void)> IsInReplay;
std::optional<bool(*)(void)> IsInRender;

struct RecordEntry{
    float timestamp;
    unsigned int heartrate;
};
std::vector<RecordEntry> recordData;
std::string heartDeviceName = HEART_DEV_NAME_UNK;

//this callback is called by beatleader when game end
void RecordCallback(std::string name, int* length, void** data){
    if(recordStarted == false)
        return;
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

// ReplayCallback(HeartBeatQuest) -> ReplayCallback(HRCounter) -> SinglePlayerInstallBindings(ReplayCallbackShouldCleanData=true) -> Play the replay in game play
bool ReplayCallbackShouldCleanData = true;

//this callback is called by replay mod
void ReplayCallback(const char * buff, size_t length){
    recordStarted = false;// just make sure we have no bugs, it's already true here.

    if(ReplayCallbackShouldCleanData){
        ReplayCallbackShouldCleanData = false;
        replayStarted = false;
        recordData.clear();
    }else{
        ReplayCallbackShouldCleanData = true;
    }

    if(buff == nullptr || length == 0){
        getLogger().info("no replay data detected, or length is zero.");
        return;
    }

    //feed the recordData
    int i = 0;
    auto GetUInt32 = [&](unsigned int &x){
        uint8_t *d = (uint8_t *)&x;
        if(i + 4 >= length) return false;
        d[0] = buff[i++];
        d[1] = buff[i++];
        d[2] = buff[i++];
        d[3] = buff[i++];
        x = le32toh(x);
        return true;
    };
    auto GetFloat = [&](float &x){
        uint8_t *d = (uint8_t *)&x;
        if(i + 4 >= length) return false;
        d[0] = buff[i++];
        d[1] = buff[i++];
        d[2] = buff[i++];
        d[3] = buff[i++];
        return true;
    };
    auto GetStr = [&](std::string & ret){
        unsigned int len;
        if(!GetUInt32(len))
            return false;
        if(i + len >= length)
            return false;
        ret = std::string(&buff[i], len);
        i += len;
        return true;
    };
    
    unsigned int ver;
    if(!GetUInt32(ver))
        return;
    getLogger().info("replay data detected, version {}.", ver);

    if(ver != 1){
        getLogger().info("the replay data version is not supported.");
        return;
    }
    unsigned int recordCount;
    if(!GetUInt32(recordCount))
        return;
    recordData.clear();
    replayStarted = true;
    currentDataToReplay = -1;
    for(int i=0;i<recordCount;i++){
        float timestamp;
        unsigned int heartrate;
        if(!GetFloat(timestamp))
            return;
        if(!GetUInt32(heartrate))
            return;
        // getLogger().info("timestamp {}, data {}", timestamp, heartrate);
        recordData.emplace_back(timestamp, heartrate);
    }
    getLogger().info("{} heart rate data loaded.", recordData.size());


    //actually we don't care about it
    // std::string devName;
    // if(!GetStr(devName))
    //     return;
}

GlobalNamespace::AudioTimeSyncController* audioTimeSyncController = NULL;
MAKE_HOOK_MATCH(ScoreControllerStart, &GlobalNamespace::ScoreController::Start, void, GlobalNamespace::ScoreController* self) {
    ScoreControllerStart(self);
    audioTimeSyncController = self->_audioTimeSyncController;
}

MAKE_HOOK_MATCH(SinglePlayerInstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void, GlobalNamespace::GameplayCoreInstaller* self) {
    SinglePlayerInstallBindings(self);

    ReplayCallbackShouldCleanData = true;

    auto DisableRecord = [](){
        recordStarted = false;
        isPaused = false;
        recordData.clear();
    };

    if((IsInReplay.has_value() && IsInReplay.value()()) || (IsInRender.has_value() && IsInRender.value()()) || replayStarted){
        if(needReplay){
            //don't clear the recordData, we need replay them
            recordStarted = false;
            isPaused = false;    
        }else{
            DisableRecord();
        }
        getLogger().info("this is a replay, don't start record. replaying = {}", replayStarted);
        // the replay data should have been loaded by replay callback now.
        return;
    }else{
        recordData.clear();
        replayStarted = false;
    }

    if(!needRecord)
        return;

    if(dataSourceType == DS_RANDOM){
        DisableRecord();
        getLogger().info("random datasource will not enable record.");
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
    }

    auto AddReplayCustomDataCallback = CondDeps::FindUnsafe<void, std::string, std::function<void(const char*, size_t)> >("replay", "AddReplayCustomDataCallback");
    if(AddReplayCustomDataCallback.has_value()){
        getLogger().info("Replay mod is detected, enable replay support");
        needReplay = true;
        AddReplayCustomDataCallback.value()("HeartBeatQuest", ReplayCallback);
        AddReplayCustomDataCallback.value()("HRCounter", ReplayCallback);

    }
    IsInReplay = CondDeps::FindUnsafe<bool>("replay", "IsInReplay");
    IsInRender = CondDeps::FindUnsafe<bool>("replay", "IsInRender");

    if(needRecord || needReplay){
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
    //auto beg_time = now_ms();

    auto isInSection = [](int index){
        return index >= 0 && index < recordData.size() && recordData[index].timestamp <= audioTimeSyncController->songTime &&   
            (index + 1 >= recordData.size() || recordData[index+1].timestamp > audioTimeSyncController->songTime);
    };
    if(replayStarted && audioTimeSyncController){
        if(isInSection(currentDataToReplay)){
            // getLogger().info("in section");
            return false;
        }
        if(isInSection(currentDataToReplay+1)){
            currentDataToReplay++;
            heartrate = recordData[currentDataToReplay].heartrate;
            return true;
        }

        //this only happens when player changes the replay progress
        getLogger().info("search from {} datas", recordData.size());
        for(int i=0;i<recordData.size();i++){
            //we don't need a binary search  
            if(isInSection(i)){
                currentDataToReplay = i;
                heartrate = recordData[currentDataToReplay].heartrate;
                //auto done_time = now_ms();
                //getLogger().debug("search done in {}ms", done_time - beg_time);
                return true;    
            }
        }
    }
    return false;
}

}
}
