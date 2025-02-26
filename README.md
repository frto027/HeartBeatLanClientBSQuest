# HeartBeatLanReceiver - Quest (BeatSaber mod)

[![QMOD BUILD](https://github.com/frto027/HeartBeatLanClientBSQuest/actions/workflows/qmod_build.yml/badge.svg)](https://github.com/frto027/HeartBeatLanClientBSQuest/actions/workflows/qmod_build.yml)

[中文说明](README.cn.md)

the latest version for beatsaber `1.37.0_9064817954`

view your heartbeat inside the quest game

1. patch the game with the `bluetooth` permission via [mbf](https://mbf.bsquest.xyz/), which is the recommand way to mod the beatsaber now.
1. install this mod via mbf.
2. In your quest bluetooth setthings, pair your heart rate BLE device with your quest.
3. Open the game, scan and select your device at the device list menu.

More accurately, it requires the game has the following permission


        android.permission.BLUETOOTH
        android.permission.BLUETOOTH_CONNECT

# Alternative way

> OSC Protocol is supported, bluetooth permission is not required

Use your favorite heart rate OSC senders, send to the port 9000 for your quest device.

The port will be show at the mod setthings menu, and can edit via config file. 

You can also use [this android apk](https://github.com/frto027/HeartbeatLanServer/releases/latest) to send osc data from your android phone, or install it on your quest device directly and send to `127.0.0.1:9000`.

# Alternative way(don't use it if other way works for you)
<details>
        
> An android application to read heart rate data is also avaliable

If you can't or don't want to patch your game with bluetooth permission, you can use [this android apk](https://github.com/frto027/HeartbeatLanServer/releases/latest) in your quest or android phone in same local network. After you use it, switch the data source toggle to `local network` inside the game at the mod menu in your left side.

</details>

# How it works

This mod uses [JNI](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html) to load an external java library with [PathClassLoader](https://developer.android.com/reference/dalvik/system/PathClassLoader) to access bluetooth device.

# Dev.
Use `qpm s build` to build
Same goes for `qpm s copy` and `.\scripts\createqmod.ps1`

# Dev. Android Project

The Bluetooth access code at `AndroidProject/HeartBeatNative` can be compiled with AndroidStudio and Android SDK 34.

# Mod api


## Basic API Usage

Copy `shared/HeartBeatApi.h` file to your mod

The file is clean, and only depends on `scotland2` mod loader.

for example in your mod:

```cpp
#include "HeartBeatApi.h"

HeartBeatApi * heartBeatApi = nullptr;
extern "C" void late_load() {
    heartBeatApi = HeartBeat::GetHeartBeatApi();
}

void Update(){
    if(heartBeatApi){
        heartBeatApi->Update();
        int data;
        if(heartBeatApi->GetData(&data)){
            //new data come in this frame
        }else{
            //old buffered data got
        }
        // the result of GetData will not chaned until you call heartBeatApi->Update() at next game update cycle
    }
}
```


## Replay Structure

The mod records hearts data to replay file with mod ID `HeartBeatQuest`.

little endian, 4 byte int, 4 byte float, string(length+bytes), which follows [BSOR](https://github.com/BeatLeader/BS-Open-Replay) format

```
version                 - int, the value is always 1
dataCount               - int, how many data we will have later

{                       - heart rate datas, repeat dataCount times
  time                  - float, timestamp
  heartRate             - int, heart rate data
}

bluetoothDeviceName     - string, a utf-8 format byte array directly from from Java VM
```

## Credits

This mod is created by frto027.

* [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
* [raftario](https://github.com/raftario)
* [Lauriethefish](https://github.com/Lauriethefish), [danrouse](https://github.com/danrouse) and [Bobby Shmurner](https://github.com/BobbyShmurner) for [this template](https://github.com/Lauriethefish/quest-mod-template)
