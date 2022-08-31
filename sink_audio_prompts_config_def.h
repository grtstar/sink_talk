
/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_audio_prompts_config_def.h

DESCRIPTION 
    This file contains the module specific configuration structure(s) and
    configuration block id(s) to be used with the config_store library.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#ifndef _SINK_AUDIO_PROMPTS_CONFIG_DEF_H_
#define _SINK_AUDIO_PROMPTS_CONFIG_DEF_H_

#include "config_definition.h"

typedef struct {
    unsigned event;
    unsigned state_mask:14;
    unsigned sco_block:1;
    unsigned cancel_queue_play_immediate:1;
    unsigned prompt_id:8;
    unsigned padding:8;
} audio_prompts_config_type_t;

#define SINK_AUDIOPROMPTS_READONLY_CONFIG_BLK_ID 598

typedef struct {
    unsigned LanguageConfirmTime_s;
    unsigned num_audio_prompt_languages;
    unsigned DisableAudioPromptTerminate:1;
    unsigned VoicePromptNumbers:1;
    unsigned VoicePromptPairing:1;
    unsigned padding:13;
} sink_audioprompts_readonly_config_def_t;

#define SINK_AUDIOPROMPTS_WRITEABLE_CONFIG_BLK_ID 605

typedef struct {
    unsigned num_audio_prompts;
    unsigned audio_prompt_language:4;
    unsigned audio_prompts_enabled:1;
    unsigned padding:11;
} sink_audioprompts_writeable_config_def_t;

#define SINK_AUDIOPROMPTS_CONFIG_BLK_ID 611

typedef struct {
    audio_prompts_config_type_t audioPromptEvents[1];
} sink_audioprompts_config_def_t;

#endif /* _SINK_AUDIO_PROMPTS_CONFIG_DEF_H_ */
