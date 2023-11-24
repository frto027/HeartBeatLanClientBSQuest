#include "HeartBeatDataSource.hpp"

namespace HeartBeat{
    DataSource* DataSource::getInstance(){
        return getInstance<HeartBeatLanDataSource>();
    }

    bool DataSource::GetData(int&heartbeat){
        return false;
    }
    void DataSource::Update(){

    }
}

