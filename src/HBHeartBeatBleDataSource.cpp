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

HeartBeat::HeartBeatBleDataSource * bleDataSource;

JNIEnv * env;
jobject bleReader;
jmethodID bleReader_BleStart;
jmethodID bleReader_BleToggle;
jmethodID bleReader_IdDeviceSelected;

JNIEXPORT void JNICALL
Java_top_zxff_nativeblereader_BleReader_OnDeviceData
(JNIEnv *env, jobject thiz, jstring macAddr, jint heartRate, jlong energy){
    //this happens on a background thread, as google documented
    auto chars = env->GetStringUTFChars(macAddr, NULL);
    bleDataSource->OnDataCome(chars, heartRate, energy);
    env->ReleaseStringUTFChars(macAddr, chars);
}
JNIEXPORT void JNICALL
Java_top_zxff_nativeblereader_BleReader_InformNativeDevice(JNIEnv *env, jobject thiz, jstring macAddr, jstring name){
    //Add the ui or do something...
    //bleReader_BleStart call this function in java code
    auto macChar = env->GetStringUTFChars(macAddr, NULL);
    auto nameChar = env->GetStringUTFChars(name, NULL);
    bleDataSource->InformNativeDevice(macChar, nameChar);
    env->ReleaseStringUTFChars(macAddr, macChar);
    env->ReleaseStringUTFChars(name, nameChar);
}

void ScanDevices(){
    //also check permissions
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
    // use jni to load a java library to access bluetooth devices

    auto ret = modloader_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    
    if(env == nullptr){
        getLogger().debug("JNI Env is nullptr");
        return;
    }
    env->ExceptionClear();

    /*
    Good, will load my java class

        new dalvik.system.PathClassLoader({path}, {env->FindClass("com/unity3d/player/UnityPlayer").getClassLoader()})
            .loadClass("top.zxff.nativeblereader.BleReader")
            
    */
    auto path_str = env->NewStringUTF(path.c_str());

    auto unityPlayerClass = env->FindClass("com/unity3d/player/UnityPlayer");
    if(unityPlayerClass == nullptr){
        getLogger().error("UnityPlayer class not found");
        return;
    }

    auto ClassClass = env->FindClass("java/lang/Class");
    auto ClassClass_getClassLoader = env->GetMethodID(ClassClass, "getClassLoader", 
        "()Ljava/lang/ClassLoader;");
    if(ClassClass_getClassLoader == nullptr){
        getLogger().error("Can't find class loader");
        return;
    }
    auto ClassLoader = env->CallObjectMethod(unityPlayerClass, ClassClass_getClassLoader);
    
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

    // We can't use env->FindClass("top.zxff.nativeblereader.BleReader") to find our class
    // Because we use a different class loader, which is a PathClassLoader,
    //      and FindClass uses the classloader that ralated to the top of call stack
    auto bleReaderClass = static_cast<jclass>(return_value_of_loadClass);
    if(bleReaderClass == nullptr){
        getLogger().error("class not found");
        return;
    }
    //find class method
    bleReader_BleStart = env->GetMethodID(bleReaderClass, "BleStart", "()V");
    bleReader_BleToggle = env->GetMethodID(bleReaderClass, "BleToggle", "(Ljava/lang/String;Z)Z");
    bleReader_IdDeviceSelected = env->GetMethodID(bleReaderClass, "IsDeviceSelected","(Ljava/lang/String;)Z");

    static const JNINativeMethod methods[] = {
        {"OnDeviceData", "(Ljava/lang/String;IJ)V", reinterpret_cast<void*>(Java_top_zxff_nativeblereader_BleReader_OnDeviceData)},
        {"InformNativeDevice", "(Ljava/lang/String;Ljava/lang/String;)V", reinterpret_cast<void*>(Java_top_zxff_nativeblereader_BleReader_InformNativeDevice)},
    };
    int rc = env->RegisterNatives(bleReaderClass, methods, sizeof(methods)/sizeof(JNINativeMethod));
    if (rc != JNI_OK){
        getLogger().error("Failed to register native methods");
        return;
    }


    auto bleReaderCtor = env->GetMethodID(bleReaderClass, "<init>", "()V");
    bleReader = env->NewGlobalRef(env->NewObject(bleReaderClass, bleReaderCtor));

    if(env->ExceptionCheck()){
        getLogger().debug("Exception occurred");
        env->ExceptionDescribe();
        return;
    }

    getLogger().debug("Java module loaded {} {} {}", (void*)bleReader_BleStart, (void*)bleReader_BleToggle, (void*)bleReader_IdDeviceSelected);
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
