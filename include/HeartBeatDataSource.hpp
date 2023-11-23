#include "sys/socket.h"
#include "sys/types.h"
#include <vector>
#include <string>
#include <map>

namespace HeartBeat{

class DataSource{
public:
    virtual bool GetData(int& heartbeat);
    virtual void Update();
    static DataSource* getInstance();
};


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
    HeartBeatLanDevice * latest_data_dev = nullptr;
public:
    HeartBeatLanDataSource();
    bool GetData(int& heartbeat);
    void StartPair();
    void Update();
    void StopPair();

    const char * GetProtocolVersion();
    std::vector<HeartBeatLanDataSourceServer> paired_servers;
    std::map<std::string/*mac*/, HeartBeatLanDevice> avaliable_devices;
};
};//namespace HeartBeat