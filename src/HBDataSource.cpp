#include "HeartBeatDataSource.hpp"

namespace HeartBeat{
    DataSource* DataSource::getInstance(){
        static RandomDataSource r;
        return &r;
    }

    bool DataSource::GetData(int&heartbeat){
        return false;
    }
    void DataSource::Update(){

    }
}