#include "sink_a2dp.h"
#include "sink_anc.h"
#include "sink_configmanager.h"
#include "sink_devicemanager.h"
#include "sink_config.h"
#include "sink_wired.h"
#include "sink_usb.h"
#include "sink_slc.h"
#include "sink_auth.h"
#include "sink_pio.h"
#include "sink_avrcp.h"
#include "sink_device_id.h"
#include "sink_link_policy.h"
#include "sink_powermanager.h"
#include "sink_peer_qualification.h"
#include "sink_audio.h"
#include "sink_audio_routing.h"
#include "sink_music_processing.h"
#include "sink_inquiry.h"
#include "sink_volume.h"
#include "sink_main_task.h"
#include "sink_malloc_debug.h"
#include "sink_devicemanager.h"
#include "sink_device_id.h"
#include "sink_events.h"
#include "sink_debug.h"
#include "sink_audio_prompts.h"
#include "sink_statemanager.h"
#include "sink_scan.h"

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

#include "headset_multi_talk.h"
#include "audio_prompt.h"
#include "csr_common_example_plugin.h"

void handleMTL2capConnectIndFriendMode(CL_L2CAP_CONNECT_IND_T *msg)
{
}

void handleMTL2capConnectCfmFriendMode(CL_L2CAP_CONNECT_CFM_T *msg)
{
}

void handleMTL2capDisconIndFriendMode(CL_L2CAP_DISCONNECT_IND_T *msg)
{
}

void handleMTL2capDisconCfmFriendMode(CL_L2CAP_DISCONNECT_CFM_T *msg)
{
}

void handleMTSynConnIndFriendMode(CL_DM_SYNC_CONNECT_IND_T *msg)
{
}

void handleMTSynConnCfmFriendMode(CL_DM_SYNC_CONNECT_CFM_T *msg)
{
}

void handleMTSynDisconIndFriendMode(CL_DM_SYNC_DISCONNECT_IND_T *msg)
{
}

bool processEventMultiTalkFriendMode(Task task, MessageId id, Message message)
{
    bool ret = FALSE;
    switch (id)
    {
    case EventMultiTalkKeyPress:
        MT_DEBUG(("MT: Enter Pair\n"));
        mt->status = MT_ST_PARING;
        AudioPlay(AP_MULTI_TALK_PAIRING, TRUE);
        inquiryPair(inquiry_session_multi_talk, TRUE);
        mtDisconnect();
        mtClearRouteTable(&mt->route_table);
        RouteTablePushChild(&mt->route_table, mt->addr);
        mt->total_connected = 1;
        mt->mt_type = MT_NODE;
        MessageCancelAll(task, EventMultiTalkConnect);
        MessageCancelAll(task, EventMultiTalkReconnect);
        MessageCancelAll(task, EventSysMultiTalkReconnect);
        deviceManagerRemoveAllDevices();
        mtResetPairList();
        mt->connected_device = RouteTableGetIndex(&mt->route_table, &mt->addr);
        PowerAmpOn();
        break;
    case EventMultiTalkKeyClick:
        break;
    }
}