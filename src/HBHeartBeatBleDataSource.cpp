#include "HeartBeatDataSource.hpp"
#include <cstddef>
#include <jni.h>
#include <string>
#include <utility>
#include <vector>
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "main.hpp"
#include "scotland2/shared/modloader.h"

namespace HeartBeat{
    DECLARE_DATA_SOURCE(HeartBeatBleDataSource)
}

class EnvAttacher{
    bool detached;
    JNIEnv** envp;
public:
    EnvAttacher(bool detached, JNIEnv** envp):detached(detached),envp(envp){
        if(detached)
            modloader_jvm->AttachCurrentThread(envp, NULL);
    }

    ~EnvAttacher(){
        // if(detached)
        //     modloader_jvm->DetachCurrentThread();
    }
};

HeartBeat::HeartBeatBleDataSource * bleDataSource;

JNIEnv * env;
jobject bleReader;
jclass bleReaderClass;
jmethodID bleReader_BleStart;
jmethodID bleReader_BleToggle;
jmethodID bleReader_IdDeviceSelected;

JNIEXPORT void JNICALL
Java_top_zxff_nativeblereader_BleReader_OnDeviceData
(JNIEnv *env, jobject thiz, jstring macAddr, jint heartRate, jlong energy){
    //this happens on a background thread
    auto chars = env->GetStringUTFChars(macAddr, NULL);
    bleDataSource->OnDataCome(chars, heartRate, energy);
    env->ReleaseStringUTFChars(macAddr, chars);
}
JNIEXPORT void JNICALL
Java_top_zxff_nativeblereader_BleReader_InformNativeDevice(jstring macAddr, jstring name){
    //Add the ui or do something...
    auto macChar = env->GetStringUTFChars(macAddr, NULL);
    auto nameChar = env->GetStringUTFChars(name, NULL);
    bleDataSource->InformNativeDevice(macChar, nameChar);
    env->ReleaseStringUTFChars(macAddr, macChar);
    env->ReleaseStringUTFChars(name, nameChar);
}

void ScanDevices(){
    env->CallVoidMethod(bleReader, bleReader_BleStart);
    if(env->ExceptionCheck()){
        getLogger().debug("Exception occurred");
        env->ExceptionDescribe();
        return;
    }
}

bool ToggleDevice(std::string macAddr, jboolean selected){
    auto ret = env->CallBooleanMethod(bleReader, bleReader_BleToggle, env->NewStringUTF(macAddr.c_str()),selected);
    if(env->ExceptionCheck()){
        getLogger().debug("Exception occurred");
        env->ExceptionDescribe();
        return false;
    }
    return ret;
}

bool IsDeviceSelected(std::string macAddr){
    auto ret = env->CallBooleanMethod(bleReader, bleReader_IdDeviceSelected, env->NewStringUTF(macAddr.c_str()));
        if(env->ExceptionCheck()){
        getLogger().debug("Exception occurred");
        env->ExceptionDescribe();
        return false;
    }
    return ret;
}

void LoadJavaLibrary(std::string path){
    //We use jni to load a java library to access bluetooth devices
    // il2cpp_utils::threading::attach_thread();
    auto ret = modloader_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    
    getLogger().debug("jni GetEnv()->{}", ret);
    if(env == nullptr){
        getLogger().debug("JNI Env is nullptr");
        return;
    }
    env->ExceptionClear();

    //Good, I will load my java class
    auto path_str = env->NewStringUTF(path.c_str());
    getLogger().debug("Finding class loader");
    auto ClassLoaderClass = env->FindClass("java/lang/ClassLoader");
    getLogger().debug("Get SystemClassLoader, class loader {}", (void*)ClassLoaderClass);
    auto ClassLoader_getSystemClassLoader = env->GetStaticMethodID(ClassLoaderClass, "getSystemClassLoader", 
        "()Ljava/lang/ClassLoader;");
    getLogger().debug("Method A ID {}", (void*)ClassLoader_getSystemClassLoader);
    if(ClassLoader_getSystemClassLoader == nullptr)
        return;
    auto ClassLoader = env->CallStaticObjectMethod(ClassLoaderClass, ClassLoader_getSystemClassLoader);
    
    auto PathClassLoaderClass = env->FindClass("dalvik/system/PathClassLoader");
    if(PathClassLoaderClass == nullptr){
        getLogger().error("can't find dalvik.system.PathClassLoader");
        return;
    }
    auto PathClassLoaderInit = env->GetMethodID(PathClassLoaderClass, "<init>", "(Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    if(PathClassLoaderInit == nullptr){
        getLogger().debug("Enpty LoaderInit");
        return;
    }
    auto PathClassLoader = env->NewObject(PathClassLoaderClass, PathClassLoaderInit, path_str, ClassLoader);

    auto LoadClassMethod = env->GetMethodID(PathClassLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    auto return_value_of_loadClass = env->CallObjectMethod(PathClassLoader, LoadClassMethod, env->NewStringUTF("top.zxff.nativeblereader.BleReader"));
    if(env->ExceptionCheck()){
        getLogger().debug("Exception occurred");
        env->ExceptionDescribe();
        return;
    }

    auto return_value_of_loadClass_class = env->GetObjectClass(return_value_of_loadClass);

    auto getMethodMethod = env->GetMethodID(return_value_of_loadClass_class,
        "getMethod", "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
    auto return_value_of_getMethod = env->CallObjectMethod(return_value_of_loadClass, getMethodMethod,
        env->NewStringUTF("Initialize"), env->NewObjectArray(1, env->FindClass("java/lang/Class"), env->FindClass("java/lang/Class")));
    
    if(env->ExceptionCheck()){
        getLogger().debug("Exception occurred");
        env->ExceptionDescribe();
        return;
    }

    auto unityPlayerClass = env->FindClass("com/unity3d/player/UnityPlayer");
    if(unityPlayerClass == nullptr){
        getLogger().error("UnityPlayer class not found");
        return;
    }

    auto return_value_of_getMethod_class = env->GetObjectClass(return_value_of_getMethod);

    auto invokeMethod = env->GetMethodID(return_value_of_getMethod_class, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    auto tmpBleReader = env->CallObjectMethod(return_value_of_getMethod, invokeMethod, nullptr,env->NewObjectArray(1, env->FindClass("java/lang/Object"), unityPlayerClass));
    if(env->ExceptionCheck()){
        getLogger().debug("Exception occurred");
        env->ExceptionDescribe();
        return;
    }

    bleReaderClass = env->GetObjectClass(tmpBleReader);

    //find class method
    bleReader_BleStart = env->GetMethodID(bleReaderClass, "BleStart", "()V");
    bleReader_BleToggle = env->GetMethodID(bleReaderClass, "BleToggle", "(Ljava/lang/String;Z)Z");
    bleReader_IdDeviceSelected = env->GetMethodID(bleReaderClass, "IsDeviceSelected","(Ljava/lang/String;)Z");

    // recreate the object with JNI to prevent crash
    auto bleReaderCtor = env->GetMethodID(bleReaderClass, "<init>", "(Ljava/lang/Class;)V");
    bleReader = env->NewObject(bleReaderClass, bleReaderCtor, unityPlayerClass);
    bleReader = env->NewGlobalRef(bleReader);

    getLogger().debug("Loaded! {} {} {}", (void*)bleReader_BleStart, (void*)bleReader_BleToggle, (void*)bleReader_IdDeviceSelected);
}


HeartBeat::HeartBeatBleDataSource::HeartBeatBleDataSource(){
    bleDataSource = this;
    LoadJavaLibrary("/sdcard/ModData/com.beatgames.beatsaber/Mods/classes.dex");
}

void HeartBeat::HeartBeatBleDataSource::SetSelectedBleMac(const std::string mac){ 
    ToggleDevice(this->selected_mac, false);

    this->selected_mac = mac;
    getModConfig().SelectedBleMac.SetValue(mac, true);

    ToggleDevice(this->selected_mac, true);
}

void HeartBeat::HeartBeatBleDataSource::ScanDevice(){
    avaliable_devices.clear();
    ScanDevices();
    SetSelectedBleMac(selected_mac);
}

bool HeartBeat::HeartBeatBleDataSource::GetData(int& heartbeat){
    heartbeat = this->heartbeat;
    if(has_new_data){
        has_new_data = false;
        return true;
    }
    return false;
}

long long HeartBeat::HeartBeatBleDataSource::GetEnergy(){
    return this->energy.load();
}

void HeartBeat::HeartBeatBleDataSource::InformNativeDevice(const std::string& macAddr, const std::string& name){
    if(avaliable_devices.find(macAddr) == avaliable_devices.end()){
        avaliable_devices.insert({macAddr, {
            .name = name,
            .mac = macAddr,
            .last_data = 0,
            .last_data_time = 0
        }});
    }
}
void HeartBeat::HeartBeatBleDataSource::OnDataCome(const std::string& macAddr, int heartRate, long energy){
    this->heartbeat = heartRate;
    this->has_new_data = true;
    this->energy.fetch_add(energy);
}
