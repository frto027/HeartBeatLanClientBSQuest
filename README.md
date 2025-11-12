# HeartBeatQuest (BeatSaber mod)

[![QMOD BUILD](https://github.com/frto027/HeartBeatQuest/actions/workflows/qmod_build.yml/badge.svg)](https://github.com/frto027/HeartBeatQuest/actions/workflows/qmod_build.yml) ![GitHub Release](https://img.shields.io/github/v/release/frto027/HeartBeatQuest?include_prereleases)

> [!TIP]
> This mod is for the quest platform. For PC platform you should use [HRCounter](https://github.com/qe201020335/HRCounter).

[中文简要说明](README.cn.md)

View your heart rate inside the BeatSaber quest game.  

# Usage

After you patch the mod(e.g. with [mbf](https://mbf.bsquest.xyz/), with or without `bluetooth` permission), you can configure it inside the game via a button on your left side. Change the data source and restart the game, and it works.

> [!NOTE]
> **DATA SOURCE CONFIG**  
> Please notice that the settings menu for each data source is on a **DIFFERENT** menu.

> [!NOTE]
> **HEART RATE RECORD**  
> It will automatically record your heart rate to beatleader's replay file if the beatleader-qmod is detected. You can disable this feature inside the mod setthings.

> [!TIP]
> **MOD ROADMAP**  
> If you have any suggestions about this mod, you are welcome to open an issue.

# Data sources

> [!NOTE]
> The data sources on this page are sorted alphabetically and are not recommendations. Each data source has its own advantages, so you can try them all.

Currently, there are 4 data sources can be used for this mod.  Brief Introduction:

- [Bluletooth](#bluetooth-device-as-the-data-source), pair your heart device with your quest directly. maybe less compatable.
- [HypeRate](#hyperate-as-the-heart-data-source) and [Pulsoid](#pulsoid-as-the-heart-data-source), you need install their app somewhere to get the heart rate and send it to game. Internet required.
- [OSC](#osc-as-the-heart-data-source), receive the heart data from `OSC protocol`.

## Bluetooth device as the data source

This mod can access Bluetooth directly. To use this, follow this instruction.

1. patch the game with the `bluetooth` permission via [mbf](https://mbf.bsquest.xyz/), which is the recommand way to mod the beatsaber now.
1. install this mod via mbf.
2. In your quest bluetooth setthings, pair your heart rate BLE device with your quest.
3. Open the game, scan and select your device in the device list menu.

The Bluetooth data source has minimum data latency, but may be less compatibility because it uses a generic BLE protocol to access Bluetooth devices. Your device should support heart rate broadcast via BLE protocol.

> [!NOTE]
> **PERMISSION REQUIRED**  
> More accurately, mbf is not required, the mod requires the game has the following permission
>
>
>        android.permission.BLUETOOTH
>        android.permission.BLUETOOTH_CONNECT


> [!NOTE]
> Bluetooth permission is not required if you don't use this data source.

## HypeRate as the heart data source

This mod supports the [HypeRate](https://www.hyperate.io/) as the data source. Just install the mod and change the data source to HypeRate in the setthings menu, then restart the game. Input your hyperate ID in the HYPERATE menu and it will works. (Special thanks to HypeRate for providing API support)

> [!NOTE]
> **ONLINE DATA COLLECTION**  
> This is an online service, which means I need collect some game information to make sure the service has no problem, such as [the version number](https://github.com/frto027/HeartBeatQuest/blob/4243eadcc4062ee619a6606da65a1ba4d50d91c8/src/HBHeartBeatHypeRateDataSource.cpp#L327). This data is only used to check for service errors and is usually automatically deleted within 3 days. If you don't want send these data to server, you can use other data sources. If you have trouble, please make sure your quest device is able to access [this website](https://heart.0xf7.top/).
>
> This mod use Cloudflare as the super cool and fast backend.

## OSC as the heart data source

Use your favorite heart rate OSC senders, send to the port 9000 for your quest device.**If your sender program is not supported by this mod, please [let me know](https://github.com/frto027/HeartBeatQuest/issues).**

After you change the data source to OSC in the setthings menu, the port will be show in the menu, and can edit manually via config file if you need. 

## Pulsoid as the heart data source

This mod supports the [Pulsoid](https://pulsoid.net/) as the data source. In the mod config menu select Pulsoid as data source, then restart the game. Follow the guide inside the PULSOID menu, click Open and then connect to pulsoid inside your quest's webbrowser. Then click Done button.

You can input your token, or sign in your pulsoid account directly.(Special thanks to Pulsoid for providing API support)

> [!NOTE]
> **PULSOID WITHOUT HEARTBEATQUEST SERVER FORWARD**  
> All your heart rate data is obtained directly from pulsoid server. But your token is forwarded from HeartBeatQuest's server when first connect.
> You can also edit your mod config file directly to skip the token forward. Your game is directly connected to pulsoid server, nothing will be forwarded.

# Default UI and DIY

> [!TIP]
> **LICENSE**  
> In this Project, the files in the `UnityUI` folder except `UnityUI/Assets/TextMesh Pro`, and **only** these files are licenced under [CC0](https://creativecommons.org/public-domain/cc0/).  
> This means you can create your own UI and distribute it for free or for a fee, and you can also add a CC-BY license to your UI project.

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

> [!NOTE]
> **FELL FREE TO REQUEST UI FEATURE**  
> If you are creating a super cool UI but can't do it with current mod, fell free to open an issue.  
> If you have any ideas about the UI, also welcome to open an issue. More customization capabilities can be added in the future if someone wants it.

# Development information

## How it works for Bluetooth device

This mod uses [JNI](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html) to load an external java library with [PathClassLoader](https://developer.android.com/reference/dalvik/system/PathClassLoader) to access bluetooth device.

The Bluetooth access code at `AndroidProject/HeartBeatNative` can be compiled with AndroidStudio and Android SDK 34.

## How to build

**make sure you have cloned the submodules.**
```sh
git clone --recursive git@github.com:frto027/HeartBeatQuest.git
```

You need an environment variable `ANDROID_NDK_HOME` with proper value. Do NOT use `ndkpath.txt`.

Use `qpm restore && qpm s build` to build.

Same goes for `qpm s copy` and `qpm qmod zip`

See [multibuild](./multibuild) directory to build for old game version.

## Mod api

### Basic API Usage

Copy `shared/HeartBeatApi.h` file to your mod. It is header-only, so you don't need a hard dependency.

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

### API ChangeLog

- `0.3.6` The mod id will be changed to `HeartBeatQuest` for game version larger than `v1.40.8`(not included). Add support for the new mod id.

# Other Information

For mod version unrelated information, like Replay data format and supported game information, please refer to [wiki](https://github.com/frto027/HeartBeatQuest/wiki)

# Credits

This mod is created by frto027.

And thanks to everyone who has directly or indirectly supported this mod.

* [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
* [raftario](https://github.com/raftario)
* [Lauriethefish](https://github.com/Lauriethefish), [danrouse](https://github.com/danrouse) and [Bobby Shmurner](https://github.com/BobbyShmurner) for [this template](https://github.com/Lauriethefish/quest-mod-template)
* NSGolova - [beatleader](https://github.com/BeatLeader/beatleader-qmod) for record  and webreplay support
* And other developers is BSMG Discord channel.
* [Hyperate](https://www.hyperate.io) provide api support.
* [Pulsoid](https://pulsoid.net/) provide api support. 
