#pragma once

#include "sys/socket.h"
#include "sys/types.h"
#include <atomic>
#include <vector>
#include <string>
#include <map>
#include <mutex>

#include "ModConfig.hpp"

#define SERVER_HOST "http://heart.0xf7.top"
#define WS_SERVER_HOST "ws://heart.0xf7.top"

// #define SERVER_HOST "http://dev.heart.0xf7.top"
// #define WS_SERVER_HOST "ws://dev.heart.0xf7.top"

namespace HeartBeat{

enum DataSourceType{
    DS_RANDOM,
    DS_LAN,
    DS_BLE,
    DS_OSC,
    DS_HypeRate,
    DS_Pulsoid,
};

extern DataSourceType dataSourceType;

inline bool IsDatasourceAbleToRecord(){
    if(dataSourceType == DS_RANDOM)
        return false;
    return true;
}

class DataSource{
protected:
    bool displayWanted = false;

public:
    virtual bool GetData(int& heartbeat);
    virtual long long GetEnergy();
    virtual void Update();
    static DataSource* getInstance();
    template<typename T>
    static T* getInstance();

    //if the displayWanted is true, the server thread will try to connect network if it has one.
    //network data source will not start new connections if the displayWanted is false.
    // but will not kill the existing connection, because we want reuse it later.
    void SetDisplayWanted(bool wanted){
        displayWanted = wanted;
    }
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

    int flush_pipe[2];
    std::string selected_mac;
public:
    HeartBeatLanDataSource();
    bool GetData(int& heartbeat);
    void StartPair();
    void SocketUpdate();
    void StopPair();
    bool IsPairing() { return pair_socket >= 0; }

    static const char * GetProtocolVersion();

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
class HeartBeatOSCDataSource:public DataSource{
private:
    int recv_socket;
    volatile int the_heart;
    volatile bool has_unread_heart_data = false;

    int flush_pipe[2];
    std::string selected_addr;

    void CreateSocket();
public:
    HeartBeatOSCDataSource();
    bool GetData(int& heartbeat);

    static void * ServerThread(void *self);
    std::mutex mutex;
    
    std::set<std::string> received_addresses;
    const std::string& GetSelectedAddress(){
        std::lock_guard<std::mutex> g(mutex);
        return selected_addr; 
    }
    const void SetSelectedAddr(const std::string& mac){
        std::lock_guard<std::mutex> g(mutex);
        this->selected_addr = mac;
    }

private:
    //in background thread
    void parseOscMessage(char *&thebuff, ssize_t &sz);
};

class HeartBeatPulsoidDataSource:public DataSource{
    private:
        volatile int the_heart;
        volatile bool has_unread_heart_data = false;
        
        bool closed = false; // set to true to close the thread

        void CreateSocket();

        bool resetRequest = false;

        bool safe_pair_wanted = false;
        bool safe_pairing = false;

        bool safe_pair_done_wanted = false;
    public:
        HeartBeatPulsoidDataSource();
        bool GetData(int& heartbeat);
    
        static void * ServerThread(void *self);
        
        void ResetConnection(){
            resetRequest = true;
        }

        void RequestPair(std::string pair_str);
        void RequestSafePair();
        void CancelSafePair(){
            safe_pair_wanted = false;
            safe_pairing = false;
        }
        bool IsSafePairing(){
            return safe_pairing;
        }

        void SafePairDone(){
            safe_pair_done_wanted = true;
        }


        bool modconfig_is_dirty = false;

        // a async message from service thread to main thread.
        bool url_open_wanted = false;
        std::string url;
        std::mutex url_mutex;


        bool err_message_dirty = false;
        std::string err_message;
        std::mutex err_message_mutex;

    private:

        void err(std::string message){
            std::lock_guard<std::mutex> g(err_message_mutex);
            err_message = message;
            err_message_dirty = true;
        }
};
    

class HeartBeatHypeRateDataSource:public DataSource{
    private:
        volatile int the_heart;
        volatile bool has_unread_heart_data = false;
        
        bool closed = false; // set to true to close the thread

        void CreateSocket();

        bool resetRequest = false;


    public:
        HeartBeatHypeRateDataSource();
        bool GetData(int& heartbeat);
    
        static void * ServerThread(void *self);
        
        void ResetConnection(){
            resetRequest = true;
        }

        bool has_message_from_server = false;
        char message_from_server[256];
        std::mutex message_from_server_mutex;

    private:
};
    
class HeartBeatBleDataSource:public DataSource{
private:
    std::string selected_mac = "";
    bool has_new_data;
    int heartbeat = 0;
    std::atomic_llong energy = 0;
    std::atomic_llong persistent_energy = 0;
public:
    HeartBeatBleDataSource();
    bool GetData(int& heartbeat);
    long long GetEnergy();

    void ScanDevice();

    std::string& GetSelectedBleMac(){ return selected_mac; }
    void SetSelectedBleMac(const std::string mac);

    std::map<std::string/*mac*/, HeartBeatLanDevice> avaliable_devices;

    //called from java
    void InformNativeDevice(const std::string& macAddr, const std::string& name);
    void OnDataCome(const std::string& macAddr, int heartRate, long energy);
    void OnEnergyReset();

};

std::string CheckHypeRateWebSocketIdentity();
const char * getQuestDeviceName();
};//namespace HeartBeat