#pragma once

struct Lang{
    const char * pairing = "Pairing.. click to stop.";
    const char * not_pairing = "Not pairing.. click to start.";
    const char * hiding_mac_addr = "hiding mac address, click to show.";
    const char * showing_mac_addr = "showing mac address, click to hide";
    const char * unknown_left_quote = "Unknown(";
    const char * server_skip = "skip ";
    const char * unknown_addr_server_fmt = "%s unknown addr.";
    const char * heart_rate_lan_protocol_ver = "Heart Rate Lan Protocol Ver ";
    const char * mod_version = "Mod version " VERSION;
    const char * waiting = "Waiting...";

    const char * config_adjust_speed = "Config Adjust Speed";

    const char * text_color = "Text Color";
    const char * flash_text_color = "Flash Text Color";
    const char * flash_duration_when_text_come = "Flash duration when data come";
    const char * flash_test = "Flash Test";

    const char * line_space = "Line Space";
    const char * reset_default_line_space = "Reset Default Line Space";
};

struct ChineseLang: public Lang{
    const char * pairing = "正在配对，点击停止";
    const char * not_pairing = "配对停止，点击开始";
    const char * hiding_mac_addr = "MAC地址已隐藏，点击显示";
    const char * showing_mac_addr = "MAC地址已显示，点击隐藏";
    const char * unknown_left_quote = "未知(";
    const char * server_skip = "已跳过 ";
    const char * unknown_addr_server_fmt = "%s 未知地址。";
    const char * heart_rate_lan_protocol_ver = "Heart Rate Lan 协议版本：";
    const char * mod_version = "模组版本 " VERSION;
    const char * waiting = "等待中...";

};

namespace I18N{
    void Setup();
}

extern Lang *LANG;