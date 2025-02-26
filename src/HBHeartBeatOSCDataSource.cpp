#include "HeartBeatDataSource.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <jni.h>
#include <string>
#include <sys/endian.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <unistd.h>
#include <utility>
#include <vector>
#include "ModConfig.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "main.hpp"
#include "scotland2/shared/modloader.h"
#include "BeatLeaderRecorder.hpp"

#define DEX_PATH "/sdcard/ModData/com.beatgames.beatsaber/Mods/HeartBeatQuest/HeartBeatBLEReader.dex"

namespace HeartBeat{

DECLARE_DATA_SOURCE(HeartBeatOSCDataSource)

HeartBeatOSCDataSource * oscDataSource;

HeartBeatOSCDataSource::HeartBeatOSCDataSource(){
    Recorder::heartDeviceName = HEART_DEV_NAME_OSC;
    this->CreateSocket();
}

void HeartBeatOSCDataSource::CreateSocket(){
    this->recv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->recv_socket == -1){
        getLogger().warn("Can't create OSC receive socket");
        return;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(getModConfig().OSCPort.GetValue());
    if(-1 == bind(this->recv_socket, (sockaddr*)&addr, sizeof(addr))){
        getLogger().error("osc socket bind failed.");
        return;
    }

    pthread_t the_thread;
    pthread_create(&the_thread, NULL, HeartBeatOSCDataSource::ServerThread, this);
}

char * readOscString(char *&buff, ssize_t& size){
    char * r = buff;
    while(*buff && size > 0){
        buff++;
        size--;
    }
    if(size == 0 || *buff)
        return nullptr;//invalid package
    buff++;size--;
    while(size % 4 != 0){
        buff++;size--;
    }
    return r;
}
uint32_t readOscInt32(char *&buff, ssize_t&size){
    if(size >= 4){
        uint32_t ret = ntohl(*(uint32_t*)buff);
        buff += 4;
        size -= 4;
        return ret;
    }
    buff = nullptr;
    return 0;
}

float readOscFloat32(char*&buff, ssize_t&size){
    uint32_t r = readOscInt32(buff, size);
    if(buff == nullptr)
        return NAN;
    return *(float*)&r;
}
uint32_t readOscTimeTag(char *&buff, ssize_t&size){
    if(size >= 8){
        uint32_t ret = ntohl(*(uint32_t*)buff);
        buff += 8;
        size -= 8;
        return ret;
    }
    buff = nullptr;
    return 0;
}

void HeartBeatOSCDataSource::parseOscMessage(char *&thebuff, ssize_t &sz){

        if(sz >= 8 && strcmp(thebuff, "#bundle") == 0){
            // parse osc bundle
            thebuff += 8;
            sz -= 8;
            while(sz > 0){
                int32_t size = readOscInt32(thebuff, sz);
                if(thebuff == nullptr)
                    return;//invalid buffer
                if(size > sz){
                    thebuff = nullptr;
                    return;//buffer overflow
                }
                if(size % 4 != 0){
                    thebuff = nullptr;
                    return;
                }
                
                char * subbuff = thebuff;
                ssize_t subbuff_sz = size;
                parseOscMessage(subbuff, subbuff_sz);
                if(subbuff == nullptr){
                    thebuff = nullptr;
                    return;//invalid buffer
                }
                sz -= size;
                thebuff += size;
            }
            return;
        }
        if(thebuff[0] != '/')
            return;//unknown package
        char * addr = readOscString(thebuff, sz);
        if(addr == nullptr) return;
        char * typestr = readOscString(thebuff, sz);
        if(typestr == nullptr) return;
        if(typestr[0] != ',' || typestr[2] != '\0' || (typestr[1] != 'f' && typestr[1] != 'i'))
            return;
        int heart_rate = 0;
        if(typestr[1] == 'f'){
            heart_rate = readOscFloat32(thebuff, sz);
            if(thebuff == nullptr)
                return;
        }
        if(typestr[1] == 'i'){
            heart_rate = readOscInt32(thebuff, sz);
            if(thebuff == nullptr)
                return;
        }

        {
            std::lock_guard<std::mutex> g(this->mutex);
            if(this->selected_addr == addr){
                this->the_heart = heart_rate;
                this->has_unread_heart_data = true;
            }
            this->received_addresses.insert(addr);
        }
}

void * HeartBeatOSCDataSource::ServerThread(void *self){
    HeartBeatOSCDataSource * me = (decltype(me))self;

    while(true){
        if(me->recv_socket < 0){
            sleep(1000);
            continue;
        }
        char buff[4096];
        ssize_t sz = recvfrom(me->recv_socket, buff, sizeof(buff), 0, NULL, NULL);
        char *thebuff = buff;
        me->parseOscMessage(thebuff, sz);
    }
    return nullptr;
}

bool HeartBeatOSCDataSource::GetData(int&heartbeat){
    if(has_unread_heart_data)
    {
        has_unread_heart_data = false;
        heartbeat = the_heart;
        return true;
    }
    return false;
}

}