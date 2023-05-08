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

#include "headset_multi_comm.h"
#include "headset_multi_talk.h"

extern MTData *mt;

bool mtSend(int ch, uint8 *data, int len)
{
    return ACLSend(mt->mt_device[ch].acl_sink, data, len);
}

bool mtBroadcastHeaderAddr1(int ch, uint8 count, bdaddr *addr)
{
    uint8 d[9] = {COMM_HEADER_ADDR1};
    MT_DEBUG(("MT: mtBroadcastHeaderAddr1 to %d\n", ch));
    d[1] = count;
    BdaddrToArray(addr, &d[2]);
    return mtSend(ch, d, sizeof(d));
}

bool mtBroadcastHeaderAddr2(int ch, uint8 count, bdaddr *addr)
{
    uint8 d[9] = {COMM_HEADER_ADDR2};
    MT_DEBUG(("MT: mtBroadcastHeaderAddr2 to %d\n", ch));
    d[1] = count;
    BdaddrToArray(addr, &d[2]);
    return mtSend(ch, d, sizeof(d));
}

bool mtSendConnectToken(int ch, int token_action)
{
    uint8 d[2] = {COMM_CONNECT_TOKEN};
    MT_DEBUG(("MT: SendConnectToken to %d\n", ch));
    d[1] = token_action;
    return mtSend(ch, d, sizeof(d));
}

bool mtSendCheckLoop(int ch, int start_num)
{
    uint8 d[2] = {COMM_CHECK_LOOP};
    bool r = FALSE;
    d[1] = start_num;
    r = mtSend(ch, d, sizeof(d));
    MT_DEBUG(("MT: SendCheckLoop to %d, r=%d\n", ch, r));
    return r;
}

void ProcessData(int ch, const uint8 *data, int size)
{
    switch (data[0])
    {
    case ACLMSG_FIND_TAIL:
        if (!mtSendFindTail(!ch, data[1]))
        {
            if (data[1] == 0)
            {
            }
            else
            {
                /* find the last device */
                RouteTablePushChild(&mt->route_table, mt->addr);
                mtReportRouteTable(ch, &mt->route_table);
            }
        }
        break;
    case ACLMSG_REPORT_TABLE:
    {
        int i, count = data[1];
        mt->route_table.count = count;
        for (i = 0; i < count; i++)
        {
            mt->route_table.item[i] = ArrayToBdaddr(&data[2 + 7 * i]);
        }
        RouteTablePushChild(&mt->route_table, mt->addr);
        if (!mtReportRouteTable(!ch, &mt->route_table))
        {
            mt->total_connected = mt->route_table.count;
            /*mt->status = MT_ST_CONNECTED;*/

            mtSaveRouteTable(&mt->route_table, MT_HEAD);
            mtSendRouteTable(ch, &mt->route_table);
            MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
        }
    }
    break;
    case ACLMSG_ROUTE_TABLE:
    {
        int i, count = data[1];
        mt->total_connected = mt->route_table.count = count;
        for (i = 0; i < count; i++)
        {
            mt->route_table.item[i] = ArrayToBdaddr(&data[2 + 7 * i]);
        }
        /*mt->status = MT_ST_CONNECTED;*/
        mtSaveRouteTable(&mt->route_table, MT_NODE);
        mtSendRouteTable(!ch, &mt->route_table);
        MessageSend(mt->app_task, EventSysMultiTalkDeviceConnected, NULL);
    }
    break;
    case COMM_HEADER_ADDR1:
        mt->header_addr[0] = ArrayToBdaddr(&data[2]);
        if (!mtBroadcastHeaderAddr1(!ch, data[1] + 1, &mt->header_addr[0]))
        {
            mt->header_addr[1] = mt->addr;
            mtBroadcastHeaderAddr2(ch, data[1] + 1, &mt->header_addr[1]);
            if (mt->mt_mode == NEARBY_MODE)
            {
                mt->nearby_connected = data[1] + 1;
            }
            else
            {
                mt->total_connected = data[1] + 1;
            }
            MT_DEBUG(("HEADER1 total:%d\n", data[1] + 1));
            mt->connect_token = TOKEN_IDLE;
            if (mt->status == MT_ST_CHECKLOOP)
            {
                mt->status = MT_ST_CONNECTING;
                mtScoConnect(mt->mt_device[mt->sco_expend_dev].acl_sink);
            }
            else
            {
                MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
            }
        }
        break;
    case COMM_HEADER_ADDR2:
    {
        mt->header_addr[1] = ArrayToBdaddr(&data[2]);
        mtBroadcastHeaderAddr2(!ch, data[1], &mt->header_addr[1]);
        mt->connect_token = TOKEN_IDLE;
        if (mt->mt_mode == NEARBY_MODE)
        {
            mt->nearby_connected = data[1];
        }
        else
        {
            mt->total_connected = data[1];
        }
        MT_DEBUG(("HEADER2 total:%d\n", data[1]));
        if (mt->status == MT_ST_CHECKLOOP)
        {
            mt->status = MT_ST_CONNECTING;
            mtScoConnect(mt->mt_device[mt->sco_expend_dev].acl_sink);
        }
        else
        {
            MessageSend(mt->app_task, EventSysMultiTalkCurrentDevices, NULL);
        }
    }
    break;
    case COMM_CONNECT_TOKEN:
        if (!mtSendConnectToken(!ch, data[1]))
        {
            if (data[1] == TOKEN_HELD)
            {
                mt->connect_token = TOKEN_WAITTING;
            }
            if (data[1] == TOKEN_RELEASE)
            {
                mt->connect_token = TOKEN_IDLE;
            }
        }

        break;
    case COMM_CHECK_LOOP:
        if (mt->status == MT_ST_CHECKLOOP)
        {
            MT_DEBUG(("MT: loop disconnect\n"));
            mtACLDisconnect(!ch);
            mt->status = MT_ST_SEARCHING;
            break;
        }
        mtSendAck(ch, COMM_CHECK_LOOP);
        if (!mtSendCheckLoop(!ch, data[1] + 1))
        {
            if (!BdaddrIsSame(&mt->header_addr[0], &mt->addr))
            {
                bdaddr temp = mt->header_addr[0];
                mt->header_addr[0] = mt->addr;
                mt->header_addr[1] = temp;
            }
            MT_DEBUG(("MT: no loop\n"));
            mtBroadcastHeaderAddr1(ch, 1, &mt->addr);
        }
        break;
    case ACLMSG_ACK:
        if (data[1] == COMM_CHECK_LOOP)
        {
            MessageCancelAll(mt->app_task, EventSysMultiTalkCheckLoop);
        }
        break;
    default:
        break;
    }
}
