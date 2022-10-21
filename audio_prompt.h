#ifndef _AUDIO_PROMPT_
#define _AUDIO_PROMPT_

/*
Index 0: 欢迎使用蓝牙对讲�wav
Index 1: 关机.wav
Index 2: 电池电量�wav
Index 3: 电池电量中等.wav
Index 4: 电池电量�wav
Index 5: 多人对讲组网.wav
Index 6: 多人对讲开始连�wav
Index 7: 组网人数1�wav
Index 8: 组网人数2�wav
Index 9: 组网人数3�wav
Index 10: 组网人数4�wav
Index 11: 组网人数5�wav
Index 12: 组网人数6�wav
Index 13: 组网人数7�wav
Index 14: 组网人数8�wav
Index 15: 2人组网完�wav
Index 16: 3人组网完�wav
Index 17: 4人组网完�wav
Index 18: 5人组网完�wav
Index 19: 6人组网完�wav
Index 20: 7人组网完�wav
Index 21: 8人组网完�wav
Index 22: 多人对讲.wav
Index 23: 多人对讲已关�wav
Index 24: 双人对讲.wav
Index 25: 双人对讲配对.wav
Index 26: 双人对讲配对成功.wav
Index 27: 退出双人对讲配�wav
Index 28: 对讲已关�wav
Index 29: 对讲已断开.wav
Index 30: 当前人数1�wav
Index 31: 当前人数2�wav
Index 32: 当前人数3�wav
Index 33: 当前人数4�wav
Index 34: 当前人数5�wav
Index 35: 当前人数6�wav
Index 36: 当前人数7�wav
Index 37: 当前人数8�wav
Index 38: 取消组网.wav
*/
enum
{
    AP_POWERON,
    AP_POWEROFF,
    AP_BATTERY_HIGH,
    AP_BATTERY_MID,
    AP_BATTERY_LOW,
    AP_MULTI_TALK_PAIRING,
    AP_MULTI_TALK_CONNECT,
    AP_MULTI_TALK_1_DISCOVERD,
    AP_MULTI_TALK_2_DISCOVERD,
    AP_MULTI_TALK_3_DISCOVERD,
    AP_MULTI_TALK_4_DISCOVERD,
    AP_MULTI_TALK_5_DISCOVERD,
    AP_MULTI_TALK_6_DISCOVERD,
    AP_MULTI_TALK_7_DISCOVERD,
    AP_MULTI_TALK_8_DISCOVERD,
    AP_MULTI_TALK_2_CONNECTED,
    AP_MULTI_TALK_3_CONNECTED,
    AP_MULTI_TALK_4_CONNECTED,
    AP_MULTI_TALK_5_CONNECTED,
    AP_MULTI_TALK_6_CONNECTED,
    AP_MULTI_TALK_7_CONNECTED,
    AP_MULTI_TALK_8_CONNECTED,
    AP_MULTI_TALK_ON,
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
    AP_END
};

void AudioPlay(int event, bool queue);
void AudioPlayFinish(void);

#endif
