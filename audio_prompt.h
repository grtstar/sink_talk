#ifndef _AUDIO_PROMPT_
#define _AUDIO_PROMPT_

/*
Index 0: 欢迎使用蓝牙对讲系统.wav
Index 1: 关机.wav
Index 2: 电池电量高.wav
Index 3: 电池电量中等.wav
Index 4: 电池电量低.wav
Index 5: 配对状态.wav
Index 6: 手机1已连接.wav
Index 7: 手机2已连接.wav
Index 8: 语音呼叫.wav
Index 9: 麦克风已打开.wav
Index 10: 麦克风已关闭.wav
Index 11: 多人对讲好友模式.wav
Index 12: 多人对讲好友模式组网.wav
Index 13: 2人组网完成.wav
Index 14: 3人组网完成.wav
Index 15: 4人组网完成.wav
Index 16: 5人组网完成.wav
Index 17: 6人组网完成.wav
Index 18: 7人组网完成.wav
Index 19: 8人组网完成.wav
Index 20: 多人对讲交友模式.wav
Index 21: 多人对讲已关闭.wav
Index 22: 双人对讲.wav
Index 23: 双人对讲配对.wav
Index 24: 双人对讲配对成功.wav
Index 25: 退出双人对讲配对.wav
Index 26: 对讲已关闭.wav
Index 27: 对讲已断开.wav
Index 28: 当前人数1人.wav
Index 29: 当前人数2人.wav
Index 30: 当前人数3人.wav
Index 31: 当前人数4人.wav
Index 32: 当前人数5人.wav
Index 33: 当前人数6人.wav
Index 34: 当前人数7人.wav
Index 35: 当前人数8人.wav
Index 36: 取消组网.wav
Index 37: 音乐分享开启.wav
Index 38: 音乐分享关闭.wav
Index 39: 保存组网失败.wav
Index 40: 自动接听已打开.wav
Index 41: 自动接听已关闭.wav
Index 42: 对讲已连接.wav
Index 43: 配对失败退出双人对讲配对.wav
Index 44: 对讲记录已清除.wav
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
    AP_MULTI_TALK_QUIT_PAIR,
    AP_MUSIC_SHARE_ON,
    AP_MUSIC_SHARE_OFF,
    AP_SAVE_FREIND_FAILED,
    AP_AUTO_ANSWER_ON,
    AP_AUTO_ANSWER_OFF,
    AP_TOW_TALK_CONNECTED,
    AP_TOW_TALK_PAIR_FAILED,
    AP_RESET_PAIR_LIST,
    AP_END
};

void AudioPlay(int event, bool queue);
void AudioPlayFinish(void);


#define PROMPT_REMOTE

#endif
