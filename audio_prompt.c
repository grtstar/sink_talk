#include "sink_audio_prompts.h"
#include "sink_main_task.h"
#include "sink_statemanager.h"
#include "sink_debug.h"

#include "audio_prompt.h"

#include <audio_plugin_voice_prompts_variants.h>
#include <audio.h>
#include <stddef.h>

#include "headset_uart.h"

#define AP_DEBUG(x) 

uint8 prompt_queue[8] = {0};
uint32 head = 0, tail = 0;

void AudioPlayFinish(void)
{
    if(head > tail)
    {
        uint8 event = prompt_queue[tail++ & 0x7];
        AP_DEBUG(("AUP: queue play %d\n", event));
        AudioPlay(event, FALSE);
    }
}

void AudioPlay(int event, bool queue)
{
    if(event < 0 || event >= AP_END)
    {
        AP_DEBUG(("AUP: error %d\n", event));
        return;
    }
#ifdef PROMPT_REMOTEx
    AP_DEBUG(("AUP: Send %d\n", event));
    UartSendPrompt(event, queue);
#else
    UartSendPrompt(event, 2);
    if(!stateManagerIsReadyForAudio())
    {
        return;
    }
    if(stateManagerGetState() == deviceLimbo)
    {
        return;
    }
    
    if(IsAudioBusy())
    {
        if(queue)
        {
            prompt_queue[(head++) & 0x7] = event;
            AP_DEBUG(("AUP: queue %d\n", event));
        }
        else 
        {
            /* code */
            AP_DEBUG(("AUP: discard %d\n", event));
        }
        
    }
    else
    {
        AP_DEBUG(("AUP: play %d\n", event));
        AudioPromptPlay((TaskData *)&csr_voice_prompts_plugin, event, TRUE, FALSE);
    }

    MessageCancelAll(&theSink.task, EventSysPromptsTonesQueueCheck);
    MessageSendConditionally ( &theSink.task, EventSysPromptsTonesQueueCheck,
                               NULL ,(const uint16 *)AudioBusyPtr());
#endif
}
