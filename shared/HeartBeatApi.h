///////////////////// HeartBeatQuest api interface file, for mod version 0.2.4 //////////////////
///////////////////// copy this file to your mod if you need heart data        //////////////////
#include "scotland2/shared/loader.hpp"
#include <dlfcn.h>

namespace HeartBeatApi{
    class ApiBase;


    /*
    Basic Usage:

    HeartBeatApi::Api * api = DynamicFindMod<HeartBeatApi::Api *>();
    if(api){
        //do something with api
    }
    */
    template<typename T>
    inline T DynamicFindMod(){
        for(auto & mod : modloader::get_loaded()){
            if(mod.info.id == "HeartBeatLanReceiver"){
                return dynamic_cast<T>((ApiBase*)dlsym(mod.handle, "_ZN12HeartBeatApi3apiE"));
            }
        }
        return nullptr;
    }


    class ApiBase{
        /* get mod version string number, for example "0.2.3" */
        virtual const char * Version() = 0;
    };

    class Api: public ApiBase{
    public:
        /* call this at least once per frame, to change the result of GetData */
        /* call this more than once in the same Update frame will be ignored */
        virtual void Update() = 0;
        /* 
            heartbeat: the output of last updated heart beat value
            return val: returns if new data was come in this update frame 
         */
        virtual bool GetData(int * heartbeat) = 0;
        /*
            if the Updater is not nullptr, it will replace the internal GetData.
            then the heart mod will display heart rate provided by the Updater, instead of data from physical sensors.
            the result of GetData is also affected.

            if the Updater is nullptr, the physical data will be used.
        */
        virtual void SetAlternateDataUpdater(bool (*Updater)(int* heart_output)) = 0;
    };
}