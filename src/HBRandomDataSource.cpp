#include "HeartBeatDataSource.hpp"

namespace HeartBeat{
    DECLARE_DATA_SOURCE(RandomDataSource)

    bool RandomDataSource::GetData(int& heartbeat){
        static int x = 0;
        if(x++%13 == 0){
            heartbeat = x % 200;
            return true;
        }
        return false;
    }
}