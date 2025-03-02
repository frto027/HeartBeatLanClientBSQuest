# UI Customize Guide

1. Open the `UnityUI/Scenes/SampleScene.unity` with `Unity 2021.3.16f1`, and run the scene, you will see how default UI works.
2. Edit the `Assets/DefaultWidget/DefaultWidget.prefab`.
3. click `Assets > Build AssetBundles`, two files were generated. `AssetBundles/defaultwidget` is only for PC to preview, and `AssetBundlesAndroid/defaultwidget` is for quest use.
4. copy the `AssetBundlesAndroid/defaultwidget` and rename it to your quest folder `/sdcard/ModData/com.beatgames.beatsaber/Mods/HeartBeatQuest/UI/<somename>.bundle`

# How it works

The quest mod uses the prefab just as `HeartController.cs`.

And with the following additional behavior:

- When loading asset bundle, every asset inside it will be scanned. And the game will only uses a prefab if it found a `name:xxxx` below `info` gameobject.
- All TMP_Text fonts were replaced to the game fonts. If we don't do this, the font will invisible.
- The mod will find a Animator for root gameobject. If found, the parameters will be set.
- if a gameobject called `auto:heartrate` is found, their texts will be replaced to heart rate.
- All gameobject name below `info` is used to record your information. The `name` will display in the mod menu. The `root` will indicate where your ui is been mounted.

Avaliable `root` value:

-`energyPanelGo`
-`songProgressPanelGO`
-`relativeScoreGo`
-`immediateRankGo`

If you want another prefab, remember to change the AssetBundle options below.

![alt text](image.png)
