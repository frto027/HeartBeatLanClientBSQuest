#pragma once
#include <string>
namespace HeartBeat{
namespace Recorder{

void Init();
void RecordDataIfNeeded(int heartrate);

bool isReplaying();
bool ReplayGetData(int&heartrate);

bool BeatLeaderDetected();

#define HEART_DEV_NAME_UNK "<unknown>" // if the device name is not avaliable for the mod reason
#define HEART_DEV_NAME_HIDE "<hidden>" // the player disable the Record Device Name option in config menu
#define HEART_DEV_NAME_LAN "<lan>"     // the lan datasource has no device name supported
#define HEART_DEV_NAME_OSC "<osc>"     // the osc datasource has no device name supported
#define HEART_DEV_NAME_HYPERATE "<hyperate>"
extern std::string heartDeviceName;

extern bool replayStarted;
}
}

