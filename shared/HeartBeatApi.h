///////////////////// HeartBeatQuest api interface file, for mod version 0.3.6 //////////////////
///////////////////// copy this file to your mod if you need heart data        //////////////////

#ifndef HEART_BEAT_QUEST_H
#define HEART_BEAT_QUEST_H

#include "scotland2/shared/loader.hpp"
#include <dlfcn.h>

struct HeartBeatApi;

#ifdef __cplusplus
extern "C"{
#endif

// everything is a function, to make sure we have no align problem
struct HeartBeatApi{
    int ApiVersion; 
    int __not_used__;

    /* 
        call this at least once per frame, to change the result of GetData.
        call this more than once in the same Update frame will be ignored.
    */
    void (*Update)(void);


    /* 
        arguments:
            heartrate: the output of last updated heart rate value

        return value:
            returns a boolean value, which means if this update frame has new data
        
        if the function returns 0, an old data will be assigned to heartrate.
    */
    int (*GetData)(int * heartrate);


    /*
        argument:
            Updater: a function pointer that act as the GetData function

        provide your own data updater to heart mod.

        if the Updater is not nullptr, it will replace the internal GetData function.
        then the heart mod will display heart rate provided by this Updater, instead of
        data from physical sensors or network data sources. the result of GetData is 
        also affected. The updater will be called once per frame if someone calls Update.

        if the Updater is nullptr, the physical data will be used.
    */
    void (*SetAlternateDataUpdater)(int (*Updater)(int* heart_output));


    void (*__not_used2__[20])(void);
} __attribute__((packed,aligned(16)));

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace HeartBeat {
#endif


inline HeartBeatApi * GetHeartBeatApi(){
    for(auto & mod : modloader::get_loaded()){
        if(mod.info.id == "HeartBeatLanReceiver" || mod.info.id == "HeartBeatQuest"){
            HeartBeatApi* api = (HeartBeatApi*)dlsym(mod.handle, "heartBeatApi");
            return api;
        }
    }
    return nullptr;
}


#ifdef __cplusplus
}
#endif

#endif // HEART_BEAT_QUEST_H

