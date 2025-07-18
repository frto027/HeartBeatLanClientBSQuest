# HeartBeatQuest (BeatSaber mod)

[![QMOD BUILD](https://github.com/frto027/HeartBeatLanClientBSQuest/actions/workflows/qmod_build.yml/badge.svg)](https://github.com/frto027/HeartBeatLanClientBSQuest/actions/workflows/qmod_build.yml) ![GitHub Release](https://img.shields.io/github/v/release/Frto027/HeartBeatLanClientBSQuest?include_prereleases)

[中文说明](README.cn.md)

view your heart rate inside the quest game

# Usage

After you patch the mod(e.g. with [mbf](https://mbf.bsquest.xyz/), with or without `bluetooth` permission), you can configure it inside the game via a button on your left side. Change the data source and restart the game, and it works.

Please notice that the settings menu for each data source is on a **DIFFERENT** menu.

It will automatically record your heart rate to beatleader's replay file if the beatleader-qmod is detected. You can disable this feature inside the mod setthings.

# Data sources

Currently, there are 3 data sources can be used for this mod.

## HypeRate as the heart data source

This mod supports the [HypeRate](https://www.hyperate.io/) as the data source, and Bluetooth permission is not required if you don't use it. Just install the mod and change the data source to HypeRate in the setthings menu, then restart the game. Input your hyperate ID in the HYPERATE menu and it will works. (Special thanks to HypeRate for providing API support)

This is the easiest way to use this mod. But this is an online service, which means I need collect some game information to make sure the service has no problem, such as [the version number](https://github.com/frto027/HeartBeatLanClientBSQuest/blob/4243eadcc4062ee619a6606da65a1ba4d50d91c8/src/HBHeartBeatHypeRateDataSource.cpp#L327). This data is only used to check for service errors and is usually automatically deleted within 3 days. If you don't want send these data to server, you can use other data sources. If you have trouble, please make sure your quest device is able to access [this website](https://heart.0xf7.top/).

This mod use Cloudflare as the super cool and fast backend.

## Bluetooth device as the data source

This mod can access Bluetooth directly. To use this, follow this instruction.

1. patch the game with the `bluetooth` permission via [mbf](https://mbf.bsquest.xyz/), which is the recommand way to mod the beatsaber now.
1. install this mod via mbf.
2. In your quest bluetooth setthings, pair your heart rate BLE device with your quest.
3. Open the game, scan and select your device at the device list menu.

More accurately, mbf is not required, the mod requires the game has the following permission


        android.permission.BLUETOOTH
        android.permission.BLUETOOTH_CONNECT

The Bluetooth data source has minimum data latency, but may be less compatibility because it uses a generic BLE protocol to access Bluetooth devices. Your device should support heart rate broadcast via BLE protocol.

## OSC as the heart data source

Use your favorite heart rate OSC senders, send to the port 9000 for your quest device.**If your sender program is not supported by this mod, please [let me know](https://github.com/frto027/HeartBeatLanClientBSQuest/issues).**

After you change the data source to OSC in the setthings menu, the port will be show at the menu, and can edit manually via config file if you need. 

You can also use [this android apk](https://github.com/frto027/HeartbeatLanServer/releases/latest) to send osc data from your android phone, or install it on your quest device directly and send to `127.0.0.1:9000`. 

This apk does not extend the device compatability, because it read bluetooth data just like what the mod does in game. If your heart monitor device is not supported, you may need some other program to convert their data to OSC protocol, or try to enable something like bluetooth broadcast in your monitor device.

## (Deprecated, do not use)UDP data source
<details>
        
> An android application to read heart rate data is also avaliable

If you can't or don't want to patch your game with bluetooth permission, you can use [this android apk](https://github.com/frto027/HeartbeatLanServer/releases/latest) in your quest or android phone in same local network. After you use it, switch the data source toggle to `local network` inside the game at the mod menu in your left side.

</details>

# Default UI and DIY

The color bar under the number indicates your heart zone.

![alt text](image.png)

as a refer, here I lists what these color bar means.

**This is not training advice, use at your own risk**

- nothing: heart lower than 50%  
- 1: heart 50% - 60%, very light  
- 2: heart 60% - 70%, light  
- 3: heart 70% - 80%, moderate  
- 4: heart 80% - 90%, hard  
- 5: heart more than 90%, maximum  


This mod supports DIY Interface via Unity asset bundle, please refer to [UnityUI/Readme.md](UnityUI/Readme.md)

If you have any ideas about the UI, please open a issue. More customization capabilities can be added in the future, if someone needs it, such as binding the UI to the saber.

# Development information

## How it works for Bluetooth device

This mod uses [JNI](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html) to load an external java library with [PathClassLoader](https://developer.android.com/reference/dalvik/system/PathClassLoader) to access bluetooth device.

The Bluetooth access code at `AndroidProject/HeartBeatNative` can be compiled with AndroidStudio and Android SDK 34.

## How to build

install [vcpkg](https://github.com/microsoft/vcpkg), makesure environment variable `VCPKG_ROOT` exists.

run `vcpkg install --triplet arm64-android` before you build this project.

Use `qpm restore && qpm s build` to build.

Same goes for `qpm s copy` and `qpm qmod zip`

See [multibuild](./multibuild) directory to build for old game version.

## Mod api

### Basic API Usage

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

## Game Version Support Matrix

If you have trouble with the latest version, you can download the developer tested mod of your game version, or report it at issue.

The latest not tested version can be found at the [github action](https://github.com/frto027/HeartBeatLanClientBSQuest/actions/workflows/qmod_build.yml) page, or [pull request](https://github.com/frto027/HeartBeatLanClientBSQuest/pulls) page for the latest game version by an auto bump robot if avaliable.

 Mod Version |Compat Game Version | Developer Tested Game Version | Comment
---|---|---|---
`0.3.3`| `1.35.0` `1.37.0` `1.40.4` `1.40.6` `1.40.7` | | not tested, not released
`0.3.2`| `1.40.6` | `1.40.6`
`0.3.0`| `1.40.4`|`1.40.4`
`0.2.3` - `0.2.6`| `1.37.0` | `1.37.0`
`0.2.0` - `0.2.2`| `1.35.0`|`1.35.0`
`0.1.0` - `0.1.1`| `1.28.0`|`1.28.0`| Very early version, hard to use, don't use it

## Key Features

Feature | Supported Version
--|--
HypeRate data source | `0.2.6` or higher version
Beatleader record | `0.2.5` or higher version, with beatleader 0.8.63(for game 1.37.0) or higher version
OSC data source | `0.2.2` or higher version
Bluetooth inside the game|`0.2.1`


# Credits

This mod is created by frto027.

And thanks to everyone who has directly or indirectly supported this mod.

* [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
* [raftario](https://github.com/raftario)
* [Lauriethefish](https://github.com/Lauriethefish), [danrouse](https://github.com/danrouse) and [Bobby Shmurner](https://github.com/BobbyShmurner) for [this template](https://github.com/Lauriethefish/quest-mod-template)
* NSGolova - [beatleader](https://github.com/BeatLeader/beatleader-qmod) for record  and webreplay support
* And other developers is BSMG Discord channel.
* [Hyperate](www.hyperate.io) provide api support. 
