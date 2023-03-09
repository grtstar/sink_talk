#ifndef _HEADSET_MULTI_COMM_
#define _HEADSET_MULTI_COMM_

#include <connection.h>
#include <a2dp.h>
#include <hfp.h>
#include <stdlib.h>
#include <audio.h>
#include <audio_plugin_if.h>
#include <audio_config.h>
#include <sink.h>
#include <bdaddr.h>
#include <vm.h>
#include <config_store.h>
#include <byte_utils.h>
#include <broadcast_context.h>
#include <audio_plugin_voice_prompts_variants.h>
#include <ps.h>

enum
{
    TOKEN_IDLE,
    TOKEN_WAITTING,
    TOKEN_HELD,
    TOKEN_RELEASE
};

typedef enum COMMMSG
{
    COMM_HEADER_ADDR1 = 100,
    COMM_HEADER_ADDR2,
    COMM_CONNECT_TOKEN,
    COMM_CHECK_LOOP
} COMMMSG;

bool mtSend(int ch, uint8_t* data, int len);
bool mtBroadcastHeaderAddr1(int ch, uint8 count, bdaddr *addr);
bool mtBroadcastHeaderAddr2(int ch, uint8 count, bdaddr *addr);
bool mtSendConnectToken(int ch, int token_action);
bool mtSendCheckLoop(int ch, int startNum);
void ProcessData(int ch, const uint8_t *data, int size);
#endif
