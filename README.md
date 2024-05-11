# HeartBeatLanReceiver - Quest (BeatSaber mod)

~~[中文说明](README.cn.md)~~

view your heartbeat inside the quest game

1. install this mod via [mbf](https://mbf.bsquest.xyz/), and patch the game with the following permission.

>Maybe some permission can be remove, not tested.

        android.permission.BLUETOOTH
        android.permission.BLUETOOTH_ADMIN
        android.permission.BLUETOOTH_SCAN
        android.permission.BLUETOOTH_ADVERTISE
        android.permission.BLUETOOTH_CONNECT

2. pair your heart rate BLE device with your quest.
3. Open the game, scan and select your device at the device list menu.


# How it works

This mod uses [JNI](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html) to load an external java library with [PathClassLoader](https://developer.android.com/reference/dalvik/system/PathClassLoader) to access bluetooth device.

# Dev.
Use `qpm s build` to build
Same goes for `qpm s copy` and `.\scripts\createqmod.ps1`

# Dev. Android Project

The Bluetooth access code at `AndroidProject/HeartBeatNative` can be compiled with AndroidStudio and Android SDK 34.

## Credits

This mod is created by frto027.

* [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
* [raftario](https://github.com/raftario)
* [Lauriethefish](https://github.com/Lauriethefish), [danrouse](https://github.com/danrouse) and [Bobby Shmurner](https://github.com/BobbyShmurner) for [this template](https://github.com/Lauriethefish/quest-mod-template)
