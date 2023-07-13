#ifndef _AUDIO_PROMPT_
#define _AUDIO_PROMPT_


/*
Index 0: 欢迎使用蓝牙对讲系统_8k.sbc
Index 1: 关机_8k.sbc
Index 2: 电量充足_8k.sbc
Index 3: 电量中等_8k.sbc
Index 4: 电池电量低_8k.sbc
Index 5: 配对状态_8k.sbc
Index 6: 手机1已连接_8k.sbc
Index 7: 手机2已连接_8k.sbc
Index 8: 语音呼叫_8k.sbc
Index 9: 麦克风已打开_8k.sbc
Index 10: 麦克风已关闭_8k.sbc
Index 11: 多人对讲好友模式_8k.sbc
Index 12: 好友模式组网_8k.sbc
Index 13: 2人组网完成_8k.sbc
Index 14: 3人组网完成_8k.sbc
Index 15: 4人组网完成_8k.sbc
Index 16: 5人组网完成_8k.sbc
Index 17: 6人组网完成_8k.sbc
Index 18: 7人组网完成_8k.sbc
Index 19: 8人组网完成_8k.sbc
Index 20: 多人对讲交友模式_8k.sbc
Index 21: 多人对讲已关闭_8k.sbc
Index 22: 双人对讲模式_8k.sbc
Index 23: 双人对讲配对_8k.sbc
Index 24: 双人对讲配对成功_8k.sbc
Index 25: 退出双人对讲配对_8k.sbc
Index 26: 对讲已关闭_8k.sbc
Index 27: 对讲已断开_8k.sbc
Index 28: 当前人数1人_8k.sbc
Index 29: 当前人数2人_8k.sbc
Index 30: 当前人数3人_8k.sbc
Index 31: 当前人数4人_8k.sbc
Index 32: 当前人数5人_8k.sbc
Index 33: 当前人数6人_8k.sbc
Index 34: 当前人数7人_8k.sbc
Index 35: 当前人数8人_8k.sbc
Index 36: 当前人数超过8人_8k.sbc
Index 37: 取消组网_8k.sbc
Index 38: 音乐分享开启_8k.sbc
Index 39: 音乐分享关闭_8k.sbc
Index 40: 保存组网失败_8k.sbc
Index 41: 自动接听已打开_8k.sbc
Index 42: 自动接听已关闭_8k.sbc
Index 43: 对讲已连接_8k.sbc
Index 44: 配对失败退出双人对讲配对_8k.sbc
Index 45: 恢复出厂设置_8k.sbc
Index 46: 退出配对_8k.sbc
Index 47: 手机1已断开_8k.sbc
Index 48: 手机2已断开_8k.sbc
Index 49: 组网人数1人_8k.sbc
Index 50: 组网人数2人_8k.sbc
Index 51: 组网人数3人_8k.sbc
Index 52: 组网人数4人_8k.sbc
Index 53: 组网人数5人_8k.sbc
Index 54: 组网人数6人_8k.sbc
Index 55: 组网人数7人_8k.sbc
Index 56: 组网人数8人_8k.sbc
Index 57: 组网人数超过8人_8k.sbc
Index 58: 进入USB升级模式_8k.sbc
*/
enum
{
    AP_POWERON,
    AP_POWEROFF,
    AP_BATTERY_HIGH,
    AP_BATTERY_MID,
    AP_BATTERY_LOW,
    AP_PARING,
    AP_PHONE1_CONNECTED,
    AP_PHONE2_CONNECTED,
    AP_VOICE_DIAL,
    AP_MIC_ON,
    AP_MIC_OFF,
    AP_MULTI_TALK_FRIEND_MODE,
    AP_MULTI_TALK_FRIEND_MODE_PAIR,
    AP_MULTI_TALK_2_CONNECTED,
    AP_MULTI_TALK_3_CONNECTED,
    AP_MULTI_TALK_4_CONNECTED,
    AP_MULTI_TALK_5_CONNECTED,
    AP_MULTI_TALK_6_CONNECTED,
    AP_MULTI_TALK_7_CONNECTED,
    AP_MULTI_TALK_8_CONNECTED,
    AP_MULTI_TALK_NEARBY_MODE,
    AP_MULTI_TALK_OFF,
    AP_TWO_TALK_ON,
    AP_TWO_TALK_PAIRING,
    AP_TWO_TALK_PAIR_SUCC,
    AP_TWO_TALK_QUIT_PAIR,
    AP_TWO_TALK_OFF,
    AP_TALK_DISCONNECTED,
    AP_MULTI_TALK_1_PERSON,
    AP_MULTI_TALK_2_PERSON,
    AP_MULTI_TALK_3_PERSON,
    AP_MULTI_TALK_4_PERSON,
    AP_MULTI_TALK_5_PERSON,
    AP_MULTI_TALK_6_PERSON,
    AP_MULTI_TALK_7_PERSON,
    AP_MULTI_TALK_8_PERSON,
    AP_MULTI_TALK_9_PERSON,
    AP_MULTI_TALK_QUIT_PAIR,
    AP_MUSIC_SHARE_ON,
    AP_MUSIC_SHARE_OFF,
    AP_SAVE_FREIND_FAILED,
    AP_AUTO_ANSWER_ON,
    AP_AUTO_ANSWER_OFF,
    AP_TOW_TALK_CONNECTED,
    AP_TOW_TALK_PAIR_FAILED,
    AP_RESET_PAIR_LIST,
    AP_QUIT_PAIR,
    AP_PHONE1_DISCONNECTED,
    AP_PHONE2_DISCONNECTED,
    AP_MULTI_TALK_1_PAIRED,
    AP_MULTI_TALK_2_PAIRED,
    AP_MULTI_TALK_3_PAIRED,
    AP_MULTI_TALK_4_PAIRED,
    AP_MULTI_TALK_5_PAIRED,
    AP_MULTI_TALK_6_PAIRED,
    AP_MULTI_TALK_7_PAIRED,
    AP_MULTI_TALK_8_PAIRED,
    AP_MULTI_TALK_9_PAIRED,
    AP_DFU,
    AP_END
};

void AudioPlay(int event, bool queue);
void AudioPlayFinish(void);

#ifndef RUN_ON_M2
#define PROMPT_REMOTE
#endif
#endif
