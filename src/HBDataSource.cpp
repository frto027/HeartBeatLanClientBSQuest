#include "HeartBeatDataSource.hpp"

namespace HeartBeat{
    DataSourceType dataSourceType = DS_RANDOM;

    DataSource* DataSource::getInstance(){
        static DataSource * instance = nullptr;
        if(instance == nullptr){
            dataSourceType = (HeartBeat::DataSourceType)getModConfig().DataSourceType.GetValue();

            switch (dataSourceType) {
                case DS_RANDOM:
                    instance = getInstance<RandomDataSource>();
                    break;
                case DS_LAN:
                    instance = getInstance<HeartBeatLanDataSource>();
                    break;
                case DS_BLE:
                    instance = getInstance<HeartBeatBleDataSource>();
                    break;
            }
        }
        return instance;
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

