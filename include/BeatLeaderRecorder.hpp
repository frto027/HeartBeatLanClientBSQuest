#pragma once
#include <string>
namespace HeartBeat{
namespace Recorder{


void Init();
void RecordDataIfNeeded(int heartrate);

bool isReplaying();
bool ReplayGetData(int&heartrate);

extern std::string heartDeviceName;
}
}