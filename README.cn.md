# HeartBeatQuest - Quest一体机BeatSaber模组

最新版本支持的游戏版本：1.40.4_5283

在quest游戏中显示心率数据。

# 蓝牙设备数据源

1. 使用[mbf](https://mbf.bsquest.xyz/)安装这个模组，同时需要给游戏打`bluetooth`权限补丁。
2. 在quest的蓝牙中，将BLE心率设备进行配对（需要设备使用蓝牙广播）。
3. 打开游戏，在左侧的设备列表中选择你的设备。（第一次扫描设备会提示申请蓝牙权限）

准确来说，这一模组需要游戏具有以下权限：


        android.permission.BLUETOOTH
        android.permission.BLUETOOTH_CONNECT

# HypeRate数据源

此方式无需bluetooth权限。心率设备连接手机配置好[HypeRate](https://www.hyperate.io/)，在游戏左侧菜单选择HypeRate作为数据源后重启游戏，然后在HypeRate菜单中输入你的HypeRateID即可使用。

# OSC数据源

支持OSC协议，无需bluetooth权限。

使用其它的OSC心率数据源，发送到你的quest设备9000端口。

这个端口号会在模组设置菜单里显示，可以手动编辑模组的设置文件来修改。

你也可以选择[这个apk](https://github.com/frto027/HeartbeatLanServer/releases/latest)。也可以把它装到quest上，然后发送至`127.0.0.1:9000`。

# 备选方式（不要使用，除非其它方式不可用）
<details>
        
> 可以使用一个安卓app来读取心率数据

把[这个apk](https://github.com/frto027/HeartbeatLanServer/releases/latest)安装到你的quest设备或者同局域网的安卓设备上。

</details>

# 其它信息

有关模组原理以及自定义界面等信息，请参考英文readme文件。

# 作者信息

本模组由frto027制作，基于的模板等组件信息详见[Readme](README.md)。
