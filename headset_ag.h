
#ifndef _HEADSET_AG_H_
#define _HEADSET_AG_H_
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
#include <aghfp.h>

#include "sink_audio_routing.h"


/* Enumeration providing symbolic references to the IDs in the AT+CSRFN and +CSRFN AT commands. */
typedef enum
{
	csr2csr_callername	= 1,
	csr2csr_raw_text,
	csr2csr_sms_ind,
	csr2csr_batt_level,
	csr2csr_power_source,
	csr2csr_codecs,
	csr2csr_codec_bandwidth
} csr2csr_indicator;

typedef enum 
{
	audio_codec_cvsd 				= 0,
	audio_codec_wbs_sbc 			= 1,
	audio_codec_wbs_amr 			= 2,
	audio_codec_wbs_evrc 			= 3,
	audio_codec_auristream_2_bit_8k,
	audio_codec_auristream_4_bit_8k,
	audio_codec_auristream_2_bit_16k,
	audio_codec_auristream_4_bit_16k
} audio_codec_type ;

typedef enum
{
	auristream_mask_cvsd				= (1 << 0),
	auristream_mask_auristream_2_bit	= (1 << 1),
	auristream_mask_auristream_4_bit	= (1 << 2)
} auristream_codec_masks;

typedef enum
{
	auristream_bw_mask_8k	= (1 << 0),
	auristream_bw_mask_16k	= (1 << 1)
} auristream_bw_masks;

typedef struct 
{
/*	uint16 packets ;
	aghfpAudioParams_params hfpAudioParams ;	
    */
    
    uint16 				syncPktTypes ;
	aghfp_audio_params 	hfpAudioParams;
	bool 				audioUseDefaults;
} audio_params_t ;

/** Structure for defining SSR parameters. These reflect the parameters in the HCI_Sniff_Subrating message. */
typedef struct
{
    uint16          max_remote_latency;
    uint16          min_remote_timeout;
    uint16          min_local_timeout;
} ssr_params;

/** Structure for defining seperate SSR parameters for SLC and SCO links. */
typedef struct
{
    ssr_params      slc_params;
    ssr_params      sco_params;
} subrate_data;
	
typedef struct 
{
    TaskData task;    
	AGHFP * aghfp;
	Sink audio_sink;
	
	Task app_task;
	TaskData * audio_plugin;

	bdaddr bd_addr ;
	
    Task codec_task ;
	uint16 volume ;
	
    /* Audio and SLC connection status */
	bool connecting_slc;
	bool audio ;
	bool connected ;
    
    audio_params_t 	audio_params ;
	
    /*not needed ?*/
	audio_params_t audio_cvsd_params ;
	audio_params_t audio_auristream_params ;

	sync_link_type link_type;
	
	/* The RFCOMM sink for the SLC connection */
    Sink            rfcomm_sink;
	/** Parameters for sniff subrating for SLC and SCO links. */
    subrate_data    ssr_data;
    
	/* The audio codec to use */
	audio_codec_type	audio_codec;

	/* Bitmap of the HF's supported CSR codecs. */
	unsigned int		hf_csr_codecs;
	/* Bitmap of the HF's supported CSR codec bandwidths. */
	unsigned int		hf_csr_codec_bandwidths;
	
	/* The audio codec to negotiate */
	audio_codec_type	codec_to_negotiate;
    
    	/* The bandwidth to negotiate in the case of a CSR CODEC (provided the HF supports bandwidth negotiation). */
	auristream_bw_masks	csr_codec_bandwidth_to_negotiate;

	/* The type of negotiation to employ for the codec_to_negotiate. */
	aghfp_codec_negotiation_type negotiation_type;
    
    	/* Set if the AG to capable of negotiating codecs using the CSR method. */
	unsigned int	csr_codec_negotiation_capable:1;
	unsigned int	codec_negotiated:1;
	unsigned int	start_audio_after_codec_negotiation:1;
	unsigned int	hf_supports_csr_bw_negotiation:1;
    unsigned int    hf_supports_csr_hf_initiated_codec_negotiation:1;
    unsigned int    ag_initiated_csr_codec_negotiation:1;
    unsigned int    ag_initiated_at_csrfn_resolved:1;
	
	
} SIMPLE_T ;

int AgInit(Task task);
void AgConnect(bdaddr *addr);
void AgDisconnect(void);
bool agVoicePopulateConnectParameters(audio_connect_parameters *connect_parameters);
Sink agGetActiveScoSink(void);
bool AgIsConnected(void);
#endif
