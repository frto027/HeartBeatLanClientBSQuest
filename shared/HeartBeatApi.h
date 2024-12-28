///////////////////// HeartBeatQuest api interface file, for mod version 0.2.4 //////////////////
///////////////////// copy this file to your mod if you need heart data        //////////////////
#include "scotland2/shared/loader.hpp"
#include <dlfcn.h>

struct HeartBeatApi;

#ifdef __cplusplus
extern "C"{
#endif

// every thing is a function, to make sure we have no aligh problem
struct HeartBeatApi{
    int ApiVersion; 
    int __not_used__;
    /* call this at least once per frame, to change the result of GetData */
    /* call this more than once in the same Update frame will be ignored */
    void (*Update)(void);
    /* 
        heartrate: the output of last updated heart rate value
        return val: returns if new data was come in this update frame 
        */
    int (*GetData)(int * heartrate);
    /*
        provide your own data updater to heart mod.

        if the Updater is not nullptr, it will replace the internal GetData function.
        then the heart mod will display heart rate provided by the Updater, instead of data from physical sensors.
        the result of GetData is also affected.

        if the Updater is nullptr, the physical data will be used.
    */
    void (*SetAlternateDataUpdater)(int (*Updater)(int* heart_output));


    void (*__not_used2__[20])(void);
} __attribute__((packed,aligned(16)));

#ifdef __cplusplus
}
#endif


/*
Basic Usage:

HeartBeatApi * api = DynamicFindMod();

if(api){
    api->Update()
    int data;
    if(api->GetData(&data)){
        ...
    }
    //do something with api
}
*/

#ifdef __cplusplus
namespace HeartBeat {
#endif

inline HeartBeatApi * GetHeartBeatApi(){
    for(auto & mod : modloader::get_loaded()){
        if(mod.info.id == "HeartBeatLanReceiver"){
            HeartBeatApi* api = (HeartBeatApi*)dlsym(mod.handle, "heartBeatApi");
            return api;
        }
    }
    return nullptr;
}


#ifdef __cplusplus
}
#endif


