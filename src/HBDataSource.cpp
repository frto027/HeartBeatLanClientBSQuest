#include "HeartBeatDataSource.hpp"

namespace HeartBeat{
    DataSource* DataSource::getInstance(){
        return getInstance<HeartBeatBleDataSource>();
    }

    bool DataSource::GetData(int&heartbeat){
        return false;
    }
    long long DataSource::GetEnergy(){
        return 0;
    }
    void DataSource::Update(){

    }
}

