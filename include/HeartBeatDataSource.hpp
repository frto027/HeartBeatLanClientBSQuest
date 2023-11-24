#pragma once

#include "sys/socket.h"
#include "sys/types.h"
#include <vector>
#include <string>
#include <map>
#include <mutex>

#include "ModConfig.hpp"

namespace HeartBeat{

class DataSource{
public:
    virtual bool GetData(int& heartbeat);
    virtual void Update();
    static DataSource* getInstance();
    template<typename T>
    static T* getInstance();
};

#define DECLARE_DATA_SOURCE(T) template<> T* DataSource::getInstance(){ static T* r = nullptr; if(!r) r = new T(); return r; }


class RandomDataSource:public DataSource{
public:
    bool GetData(int& heartbeat);
};


struct HeartBeatLanDataSourceServer{
    sockaddr addr;
    time_t last_alive_time; 
    bool ignored;
};
struct HeartBeatLanDevice{
    std::string name;
    std::string mac;
    unsigned int last_data;
    time_t last_data_time;
    bool ignored = false;
};
class HeartBeatLanDataSource:public DataSource{
private:
    int pair_socket, recv_socket;
    volatile int the_heart;
    volatile bool has_unread_heart_data = false;

    std::string selected_mac;
public:
    HeartBeatLanDataSource();
    bool GetData(int& heartbeat);
    void StartPair();
    void SocketUpdate();
    void StopPair();
    bool IsPairing() { return pair_socket >= 0; }

    const char * GetProtocolVersion();

    static void * ServerThread(void *self);
    std::mutex mutex;
    std::vector<HeartBeatLanDataSourceServer> paired_servers;
    std::map<std::string/*mac*/, HeartBeatLanDevice> avaliable_devices;
    const std::string& GetSelectedBleMac(){ return selected_mac; }
    const void SetSelectedBleMac(const std::string& mac){ 
        this->selected_mac = mac;
        getModConfig().SelectedBleMac.SetValue(mac, true);
    }
};
};//namespace HeartBeat