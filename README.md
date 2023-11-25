# HeartBeatLanReceiver - Quest (BeatSaber mod)

[中文说明](README.cn.md)

view your heartbeat inside the quest game

1. Install [this](https://github.com/frto027/HeartbeatLanServer/releases/latest) apk on your quest device. Open and select the Bluetooth heartrate devs. Don't close the app while playing the game. Or you can also install it to your android phone, and keep the phone and quest headset in the same LAN.
2. Install [this](https://github.com/frto027/HeartBeatLanClientBSQuest/releases/latest) mod on your quest(via BMBF or QuestPatcher).
3. Open the beatsaber game. If you install the app in quest, or your quest and android phone in the same network, the heartrate will display.
4. (Optional) configure the devices via the config menu inside the game at left side. If there is more than one phones using the app, you can filter out the sender or devices that you want to use. Select the bluetooth device instead of `any` to prevent other possible users from LAN.

```mermaid
graph TD;
    POLAR_H10[Polar H10]
    SMART_WATCH[Shart watch, broadcast heartrate]
    BLE_DEV[other BLE heartrate devs]
    PHONE[sender, phone, quest. android apk]
    BEATSABER[receiver, beatsaber quest mod, <b>YOU ARE HERE</b>]

    POLAR_H10--bluetooth-->PHONE;
    SMART_WATCH--bluetooth-->PHONE;
    BLE_DEV--bluetooth-->PHONE;
    PHONE--LAN-->BEATSABER;
    PHONE--LAN-->...;
```

# Dev.

~~Install both this mod and apk to quest is an accident design. I don't think we should let two app communicat each other on the same device. I will try to get rid of the apk in the future.~~
Can't let the Beatsaber game read bluetooth data directly because of android permission limit. It's fine to use the helper app.

Use `qpm-rust s build` to build
Same goes for `qpm-rust s copy` and `qpm-rust s qmod`

TODO:
- Clean up the network thread when game close? If the game's process exits normally, I think it's no need to do this.


## Credits

This mod is created by frto027.

* [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
* [raftario](https://github.com/raftario)
* [Lauriethefish](https://github.com/Lauriethefish), [danrouse](https://github.com/danrouse) and [Bobby Shmurner](https://github.com/BobbyShmurner) for [this template](https://github.com/Lauriethefish/quest-mod-template)
