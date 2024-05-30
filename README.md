# HeartBeatLanReceiver - Quest (BeatSaber mod)

[中文说明](README.cn.md)

the latest version for beatsaber `1.35.0_8016709773`

view your heartbeat inside the quest game

1. install this mod via [mbf](https://mbf.bsquest.xyz/), and patch the game with the `bluetooth` permission.
2. pair your heart rate BLE device with your quest.
3. Open the game, scan and select your device at the device list menu.

More accurately, it requires the game has the following permission


        android.permission.BLUETOOTH
        android.permission.BLUETOOTH_CONNECT

# Alternative way

> OSC Protocol is supported, bluetooth permission is not required

Use your favorite heart rate OSC senders, send to the port 9000 for your quest device.

The port will be show at the mod setthings menu, and can edit via config file. 

# Alternative way(don't use it if other way works for you)

> An android application to read heart rate data is also avaliable

If you can't or don't want to patch your game with bluetooth permission, you can use [this android apk](https://github.com/frto027/HeartbeatLanServer/releases/latest) in your quest or android phone in same local network. After you use it, switch the data source toggle to `local network` inside the game at the mod menu in your left side.

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
