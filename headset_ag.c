#include "headset_ag.h"
#include "sink_debug.h"
#include "sink_events.h"
#include "sink_audio_routing.h"
#include "sink_powermanager.h"
#include "sink_hfp_data.h"

#include <audio_plugin_if.h>
#include <audio_config.h>
#include <audio_plugin_voice_variants.h>

#ifdef ENABLE_AG

extern CvcPluginTaskdata csr_cvsd_no_dsp_plugin;

/* This is used during CSR2CSR CODEC negotiation. The HF MAY specify a bandwidth
   value, if so it will be a bitmap which a single bit set determining the requested
   bandwidth, or zero if the HF is rejecting a bandwidth request from the AG. This define
   allows the absence of the bandwidth parameter to be determined. Since valid values
   are only zero or one bit set, this value represents an invalid option and can be used
   to detect that the bandwidth option was never specified. */
#define NO_BANDWIDTH_SPECIFIED (0xffff)

#define AG_CSR_SUPPORTED_FEATURES (auristream_mask_cvsd /* | auristream_mask_auristream_2_bit | auristream_mask_auristream_4_bit */)
#define AG_CSR_SUPPORTED_FEATURES_BW (auristream_bw_mask_8k /*| auristream_bw_mask_16k*/)

/*! @brief Convert from a global app level codec enumeration (audio_codec_type) to a CSR specific bitmap (auristream_codec_masks).

	@param codecIn		An 'audio_codec_type' describing a chosen codec
	@param codecOut		Pointer to a variable to store the CSR bitmap.

	This function returns true if codecIn defines a CSR codec. Returns false otherwise.

*/
bool globalCodecEnumToCsrBitmap(audio_codec_type codecIn, auristream_codec_masks *codecOut);

/*! @brief Convert from an AGHFP WBS codec (aghfp_wbs_codec) to a global app level codec enumeration (audio_codec_type) .

	@param codecIn		An 'aghfp_wbs_codec' describing a chosen codec
	@param codecOut		Pointer to a variable to store the global app level codec enumeration value.

	This function returns true if codecIn defines an AGHFP WBS codec. Returns false otherwise.

*/
bool wbsEnumToGlobalCodecEnum(aghfp_wbs_codec codecIn, audio_codec_type *codecOut);

static SIMPLE_T *SIMPLE;

/*
	Process the information contained in the AT+CSRFN command for an HF initiated CSR CODEC negotiation.
*/
static bool csr2csrHandleFeatureNegotiationHfInitiated(AGHFP *aghfp, uint16 num_csr_features, uint16 codec, uint16 bandwidth, audio_codec_type *audio_codec_selected)
{
	bool rtnVal = FALSE;
	auristream_codec_masks codec_to_negotiate_as_csr_bitmap;

	/* Sanity check that we have something useful. */
	if ((codec != 0) && (globalCodecEnumToCsrBitmap(SIMPLE->codec_to_negotiate, &codec_to_negotiate_as_csr_bitmap) != FALSE))
	{
		if (codec != codec_to_negotiate_as_csr_bitmap)
		{
			DEBUG(("CSR2CSR: Rejecting HF's codec (AT+CSRFN)\n"));

			aghfpFeatureNegotiate(aghfp, num_csr_features, csr2csr_codecs, 0, csr2csr_codec_bandwidth, bandwidth, FALSE);

			AghfpSendError(aghfp);
		}
		else
		{
			/* Codec value is ok. */

			if ((bandwidth != NO_BANDWIDTH_SPECIFIED) && (bandwidth != SIMPLE->csr_codec_bandwidth_to_negotiate))
			{
				DEBUG(("CSR2CSR: Rejecting AG's choice of bandwidth (AT+CSRFN)\n"));

				aghfpFeatureNegotiate(aghfp, num_csr_features, csr2csr_codecs, codec, csr2csr_codec_bandwidth, 0, FALSE);

				AghfpSendError(aghfp);
			}
			else
			{
				/* The codec value will be in the form of a particular bit set in the
				   value for the message, e.g. 1, 2, 4, 8, etc. To save on space in globals
				   this values is stored as part of an enumerated list. The following switch statement
				   converts to the enumerated value. */
				switch (codec)
				{
				case (auristream_mask_cvsd):
					*audio_codec_selected = audio_codec_cvsd;
					rtnVal = TRUE;
					break;
#if 0		
					case(auristream_mask_auristream_2_bit):
						switch(bandwidth)
						{
							case(auristream_bw_mask_8k):
								*audio_codec_selected = audio_codec_auristream_2_bit_8k;
								break;
							case(auristream_bw_mask_16k):
								*audio_codec_selected = audio_codec_auristream_2_bit_16k;
								break;
							default:
								/* Default to 16k if bandwidth not specified in the AT command. */
								*audio_codec_selected = audio_codec_auristream_2_bit_16k;
								break;
						}

						rtnVal = TRUE;
						break;
					case(auristream_mask_auristream_4_bit):
						switch(bandwidth)
						{
							case(auristream_bw_mask_8k):
								*audio_codec_selected = audio_codec_auristream_4_bit_8k;
								break;
							case(auristream_bw_mask_16k):
								*audio_codec_selected = audio_codec_auristream_4_bit_16k;
								break;
							default:
								/* Default to 16k if bandwidth not specified in the AT command. */
								*audio_codec_selected = audio_codec_auristream_4_bit_16k;
								break;
						}

						rtnVal = TRUE;
						break;
#endif
				default: /* Should really get here. */
					break;
				}

				/* Issue +CSRSF response if ok to do so. */
				if (rtnVal == TRUE)
				{
					aghfpFeatureNegotiate(aghfp, num_csr_features, 6, codec, 7, bandwidth, FALSE);
					AghfpSendOk(aghfp);
				}
			}
		}
	}

	return (rtnVal);
}

/*
	Process the information contained in the AT+CSRFN command for an AG initiated CSR CODEC negotiation.
*/
static bool csr2csrHandleFeatureNegotiationAgInitiated(AGHFP *aghfp, uint16 num_csr_features, uint16 codec, uint16 bandwidth, audio_codec_type audio_codec_requested)
{
	bool rtnVal = FALSE;

	if ((codec == 0) || (bandwidth == 0))
	{
		/* The HF doesn't like something. Try another negotiation.
		   This code merely sends what has already been conifgured. HF rejection of AG paramters is achieved by hacking the
		   AG code to send a known unsupported CODEC on its first attempt. This code will then sets things straight
		   and a successful CODEC connection should be achieved. */
		auristream_codec_masks codec;

		globalCodecEnumToCsrBitmap(SIMPLE->codec_to_negotiate, &codec);
		aghfpFeatureNegotiate(aghfp, (SIMPLE->hf_supports_csr_bw_negotiation ? 2 : 1), 6, codec, 7, SIMPLE->csr_codec_bandwidth_to_negotiate, TRUE);
	}
	else
	{
		/* Check that the HF agrees with the choice of CODEC. */
		/* Note that if the bandwidth parameter is not present assume 8k for CVSD and 16k for Auristream. */
		switch (audio_codec_requested)
		{
		case (audio_codec_cvsd):
			if ((codec == auristream_mask_cvsd) &&
				((bandwidth == auristream_bw_mask_8k) || (bandwidth == NO_BANDWIDTH_SPECIFIED)))
				rtnVal = TRUE;
			break;
#if 0
			case(audio_codec_auristream_2_bit_8k):
					if((codec == auristream_mask_auristream_2_bit) &&
					   ((bandwidth == auristream_bw_mask_8k) || (bandwidth == NO_BANDWIDTH_SPECIFIED))) rtnVal = TRUE;
				break;
			case(audio_codec_auristream_2_bit_16k):
				if((codec == auristream_mask_auristream_2_bit) &&
					   ((bandwidth == auristream_bw_mask_16k) || (bandwidth == NO_BANDWIDTH_SPECIFIED))) rtnVal = TRUE;
				break;
			case(audio_codec_auristream_4_bit_8k):
					if((codec == auristream_mask_auristream_4_bit) &&
					   ((bandwidth == auristream_bw_mask_8k) || (bandwidth == NO_BANDWIDTH_SPECIFIED))) rtnVal = TRUE;
				break;
			case(audio_codec_auristream_4_bit_16k):
				if((codec == auristream_mask_auristream_4_bit) &&
					   ((bandwidth == auristream_bw_mask_16k) || (bandwidth == NO_BANDWIDTH_SPECIFIED))) rtnVal = TRUE;
				break;
#endif
		default:
			break;
		}

		if (rtnVal == TRUE)
		{
			AghfpSendOk(aghfp);
		}
		else
		{

			/* Cancel this AG initiated CODEC negotation in favour of the HF's request.
			   The HF should send a fresh request. */
			SIMPLE->ag_initiated_csr_codec_negotiation = FALSE;
			/* Negotiation paths possibly crossed over with AG. Try the negotation again from the start. */
			/* Just send an ERROR and wait for the HF to initiate another CODEC negotiation exchange. */
			AghfpSendError(aghfp);
		}
	}

	return (rtnVal);
}

static void read_config(void)
{
	uint16 temp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint16 len;

	/* Set the bluetooth address of the server */
	len = PsRetrieve(1, (void *)&temp[0], 4);
	if (len > 0)
	{
		SIMPLE->bd_addr.nap = temp[0];
		SIMPLE->bd_addr.uap = (uint8)temp[1];
		SIMPLE->bd_addr.lap = (uint32)temp[2] << 16 | (uint32)temp[3];
		DEBUG(("BDADDR [%x][%x][%lx]\n", SIMPLE->bd_addr.nap, SIMPLE->bd_addr.uap, SIMPLE->bd_addr.lap)); /*temp[3] , temp[2]  )) ;											*/
	}
	else
	{ /*	 Bluetooth address not defined, use the default */
		DEBUG(("No Bluetooth Address Specified\n"));

		Panic();
	}

	/*Use Defaults*/
	DEBUG(("AUDIO_PARAMS NONE FOUND !!\n"));

	SIMPLE->audio_cvsd_params.syncPktTypes = 0x003F;
	SIMPLE->audio_cvsd_params.hfpAudioParams.bandwidth = 8000;
	SIMPLE->audio_cvsd_params.hfpAudioParams.max_latency = 16;
	SIMPLE->audio_cvsd_params.hfpAudioParams.voice_settings = 0;
	SIMPLE->audio_cvsd_params.hfpAudioParams.retx_effort = 2;

	SIMPLE->codec_to_negotiate = audio_codec_cvsd;

	/*Use Defaults*/

	SIMPLE->audio_auristream_params.syncPktTypes = 0x003F;
	SIMPLE->audio_auristream_params.hfpAudioParams.bandwidth = 4000;
	SIMPLE->audio_auristream_params.hfpAudioParams.max_latency = 16;
	SIMPLE->audio_auristream_params.hfpAudioParams.voice_settings = 0x63;
	SIMPLE->audio_auristream_params.hfpAudioParams.retx_effort = 2;

	SIMPLE->audio_codec = audio_codec_cvsd;

	/* Failed to get SSR settings so default to zero (No SSR) */
	SIMPLE->ssr_data.slc_params.max_remote_latency = 0;
	SIMPLE->ssr_data.slc_params.min_remote_timeout = 0;
	SIMPLE->ssr_data.slc_params.min_local_timeout = 0;
	SIMPLE->ssr_data.sco_params.max_remote_latency = 0;
	SIMPLE->ssr_data.sco_params.min_remote_timeout = 0;
	SIMPLE->ssr_data.sco_params.min_local_timeout = 0;
}

/*****************************************************************************/
bool wbsEnumToGlobalCodecEnum(aghfp_wbs_codec codecIn, audio_codec_type *codecOut)
{
	bool rtn_value = TRUE;

	/* Translate from WBS module codec type to Headset codec type */
	switch (codecIn)
	{
	case (wbs_codec_cvsd):
		*codecOut = audio_codec_cvsd;
		break;
	case (wbs_codec_msbc):
		*codecOut = audio_codec_wbs_sbc;
		break;
	default:
		rtn_value = FALSE;
		break;
	}

	return (rtn_value);
}

/*****************************************************************************/
bool globalCodecEnumToCsrBitmap(audio_codec_type codecIn, auristream_codec_masks *codecOut)
{
	uint16 rtnValue = TRUE;

	switch (codecIn)
	{
	case (audio_codec_cvsd):
		*codecOut = auristream_mask_cvsd;
		break;
#if 0
		case(audio_codec_auristream_2_bit_8k):
		case(audio_codec_auristream_2_bit_16k):
			*codecOut = auristream_mask_auristream_2_bit;
			break;
		case(audio_codec_auristream_4_bit_8k):
		case(audio_codec_auristream_4_bit_16k):
			*codecOut = auristream_mask_auristream_4_bit;
			break;
#endif
	default:
		*codecOut = auristream_mask_cvsd;
		rtnValue = FALSE;
		break;
	}

	return (rtnValue);
}

static void profile_handler(Task task, MessageId id, Message message)
{

	switch (id)
	{
	case AGHFP_INIT_CFM:
	{
		AGHFP_INIT_CFM_T *msg = (AGHFP_INIT_CFM_T *)message;
		DEBUG(("AGHFP_INIT_CFM\n"));
		if (msg->status == success)
		{
			SIMPLE->aghfp = msg->aghfp;
			ConnectionWriteScanEnable(hci_scan_enable_inq_and_page);
		}
		else
		{
			DEBUG(("failure : %d\n", msg->status));
			Panic();
		}
		break;
	}

	case AGHFP_SLC_CONNECT_CFM:
	{
		AGHFP_SLC_CONNECT_CFM_T *msg = (AGHFP_SLC_CONNECT_CFM_T *)message;
		DEBUG(("AGHFP_SLC_CONNECT_CFM\n"));
		if (msg->status == aghfp_success)
		{
			DEBUG(("success\n"));
			SIMPLE->connected = TRUE;

			SIMPLE->rfcomm_sink = msg->rfcomm_sink;

			MessageSend(task, EventSysAGAudioConnect, NULL);
		}
		else
		{
			DEBUG(("failure : %d\n", msg->status));
		}

		break;
	}
	case AGHFP_AUDIO_CONNECT_CFM:
	{
		AGHFP_AUDIO_CONNECT_CFM_T *msg = (AGHFP_AUDIO_CONNECT_CFM_T *)message;
		DEBUG(("AGHFP_AUDIO_CONNECT_CFM\n"));
		if (msg->status == aghfp_success)
		{ /* SCO Created, Plumb in the DSP */
			TaskData *lPlugin = NULL;

			DEBUG(("success [%x]\n", (int)msg->audio_sink));

			SIMPLE->audio = TRUE;
			SIMPLE->audio_sink = msg->audio_sink;

			/* Extract the codec negotiated if we are setting up WBS. */
			if (msg->using_wbs)
			{
				audio_codec_type codec_type = audio_codec_wbs_sbc;

				/* Translate from WBS module codec type to Headset codec type */
				if (!wbsEnumToGlobalCodecEnum(msg->wbs_codec, &codec_type))
				{
					DEBUG(("\tUnknown WBS codec.\n"));
					Panic();
				}

				SIMPLE->audio_codec = codec_type;

				/* Let the app know that a codec has been negotiated. */
				SIMPLE->codec_negotiated = TRUE;
			}

			/* Set up the appropriate plug in */
			switch (SIMPLE->audio_codec)
			{
			case (audio_codec_cvsd):
				/* Not implemented yet. Revert to CVSD with no DSP */
				lPlugin = (TaskData *)&csr_cvsd_no_dsp_plugin;
				break;
#if 0
						case(audio_codec_auristream_2_bit_8k):
						case(audio_codec_auristream_2_bit_16k):
       	    	       		lPlugin = (TaskData *)&csr_auristream_2bit_1mic_plugin;
  							break;
						case(audio_codec_auristream_4_bit_8k):
						case(audio_codec_auristream_4_bit_16k):
       	    	       		lPlugin = (TaskData *)&csr_auristream_4bit_1mic_plugin;
							break;
						case(audio_codec_wbs_sbc):
							lPlugin = (TaskData *)&csr_sbc_1mic_plugin;
							break;
						case(audio_codec_wbs_amr):
							/* Not implemented yet. */
							DEBUG(("\tAMR not yet implemented.\n"));
							Panic();
							break;
						case(audio_codec_wbs_evrc):
							/* Not implemented yet. */
							DEBUG(("\tEVRC not yet implemented.\n"));
							Panic();
							break;
#endif
			default:
				break;
			}

			audioUpdateAudioRouting();

			switch (msg->link_type)
			{
			case (sync_link_unknown):
				DEBUG(("AUD: Link = ?\n"));
				break;
			case (sync_link_sco):
				DEBUG(("AUD: Link = SCO\n"));
				break;
			case sync_link_esco:
				DEBUG(("AUD: Link = eSCO\n"));
				break;
			}

			/* Set up our sniff sub rate params for SCO */
			ConnectionSetSniffSubRatePolicy(SIMPLE->rfcomm_sink,
											SIMPLE->ssr_data.sco_params.max_remote_latency,
											SIMPLE->ssr_data.sco_params.min_remote_timeout,
											SIMPLE->ssr_data.sco_params.min_local_timeout);
		}
		else
		{
			DEBUG(("failed [%x]\n", msg->status));
		}
	}

	break;
	case AGHFP_SLC_DISCONNECT_IND:
	{
#ifdef DEBUG_DEBUG_ENABLED
		AGHFP_SLC_DISCONNECT_IND_T *msg = (AGHFP_SLC_DISCONNECT_IND_T *)message;
		DEBUG(("AGHFP_SLC_DISCONNECT_IND%d\n", msg->status));
#endif
		SIMPLE->connected = FALSE;
	}
	break;
	case AGHFP_AUDIO_DISCONNECT_IND:
		DEBUG(("AGHFP_AUDIO_DISCONNECT_IND\n"));
		SIMPLE->audio = FALSE;

		AudioDisconnect();

		/* Set up our sniff sub rate params for SLC */
		ConnectionSetSniffSubRatePolicy(SIMPLE->rfcomm_sink,
										SIMPLE->ssr_data.slc_params.max_remote_latency,
										SIMPLE->ssr_data.slc_params.min_remote_timeout,
										SIMPLE->ssr_data.slc_params.min_local_timeout);

		break;

	case AGHFP_SLC_CONNECT_IND:
	{
		DEBUG(("AGHFP_SLC_CONNECT_IND\n"));
		AghfpSlcConnectResponse(SIMPLE->aghfp, TRUE);
	}
	break;
	case AGHFP_AUDIO_CONNECT_IND:
	{
		AGHFP_AUDIO_CONNECT_IND_T *msg = (AGHFP_AUDIO_CONNECT_IND_T *)message;
		DEBUG(("AGHFP_AUDIO_CONNECT_IND\n"));

		if (SIMPLE->audio_codec == audio_codec_cvsd)
		{
			DEBUG(("AUDIOPARAMS >ESCO: [%x] [%x]\n", SIMPLE->audio_cvsd_params.syncPktTypes, SIMPLE->audio_cvsd_params.hfpAudioParams.max_latency));
			AghfpAudioConnectResponse(msg->aghfp, TRUE, SIMPLE->audio_cvsd_params.syncPktTypes, &SIMPLE->audio_cvsd_params.hfpAudioParams);
		}
		else
		{
			DEBUG(("AUDIOPARAMS >ESCO: [%x] [%x]\n", SIMPLE->audio_auristream_params.syncPktTypes, SIMPLE->audio_auristream_params.hfpAudioParams.max_latency));
			AghfpAudioConnectResponse(msg->aghfp, TRUE, SIMPLE->audio_auristream_params.syncPktTypes, &SIMPLE->audio_auristream_params.hfpAudioParams);
		}
	}
	break;

	case AGHFP_CSR_SUPPORTED_FEATURES_IND:
	{
		AGHFP_CSR_SUPPORTED_FEATURES_IND_T *msg = (AGHFP_CSR_SUPPORTED_FEATURES_IND_T *)message;

		if (SIMPLE->csr_codec_negotiation_capable)
		{
			auristream_codec_masks codec;

			/* Save the HF's CSR supported features. */
			SIMPLE->hf_csr_codecs = msg->codecs;

			/* Reset flags. */
			SIMPLE->ag_initiated_at_csrfn_resolved = FALSE;
			SIMPLE->ag_initiated_csr_codec_negotiation = FALSE;

			if (msg->codec_bandwidths_present)
			{
				SIMPLE->hf_csr_codec_bandwidths = msg->codec_bandwidths;
				SIMPLE->hf_supports_csr_bw_negotiation = 1;
				SIMPLE->hf_supports_csr_hf_initiated_codec_negotiation = 1;
			}

			aghfpCsrSupportedFeaturesResponse(msg->aghfp, 0, 0, 0, 0, 0, AG_CSR_SUPPORTED_FEATURES, msg->codec_bandwidths_present, AG_CSR_SUPPORTED_FEATURES_BW);

			/* Negotiate Auristream if a CSR codec is specified. */
			if (globalCodecEnumToCsrBitmap(SIMPLE->codec_to_negotiate, &codec) &&
				(SIMPLE->negotiation_type == aghfp_codec_negotiation_type_csr))
			{
				/* Save the fact that this is an AG initiated CSR CODEC negotiation. */
				SIMPLE->ag_initiated_csr_codec_negotiation = TRUE;
				/* Note that if the HF supports CSR CODEC bandwidth negotiation we can add it. */
				aghfpFeatureNegotiate(msg->aghfp, (SIMPLE->hf_supports_csr_bw_negotiation ? 2 : 1), 6, codec, 7, SIMPLE->csr_codec_bandwidth_to_negotiate, FALSE);
			}

			/* This signifies that the SLC establishment is complete. This is here to let
			   messages sent conditionally on this variable that they should sent
			   their message now. This situation occurs when the user clicks on a action
			   which requires an SLC, but one had not yet been established. */
			SIMPLE->connecting_slc = FALSE;
		}
		else
		{
			AghfpSendError(msg->aghfp);
		}
	}
	break;

	case AGHFP_CSR_FEATURE_NEGOTIATION_IND:
	{
		AGHFP_CSR_FEATURE_NEGOTIATION_IND_T *msg = (AGHFP_CSR_FEATURE_NEGOTIATION_IND_T *)message;
		uint16 codec = 0;
		uint16 bandwidth = NO_BANDWIDTH_SPECIFIED;
		uint16 counter1;

		DEBUG(("CSR2CSR FEAT NEG IND "));

		/* Parse the CSRFN indicator values. */
		for (counter1 = 0; counter1 < msg->num_csr_features; counter1++)
		{
			DEBUG(("%s[%d],[%d]", counter1 == 0 ? "" : ",", msg->csr_features[counter1].indicator, msg->csr_features[counter1].value));

			switch (msg->csr_features[counter1].indicator)
			{
			case (csr2csr_codecs):
				codec = msg->csr_features[counter1].value;
				break;
			case (csr2csr_codec_bandwidth):
				bandwidth = msg->csr_features[counter1].value;
				break;
			}
		}

		DEBUG(("\n"));

		if (SIMPLE->hf_supports_csr_hf_initiated_codec_negotiation &&
			!SIMPLE->ag_initiated_csr_codec_negotiation)
		{
			audio_codec_type audio_codec_selected;

			if (csr2csrHandleFeatureNegotiationHfInitiated(msg->aghfp, msg->num_csr_features, codec, bandwidth, &audio_codec_selected))
			{
				/*store the codec*/
				SIMPLE->audio_codec = SIMPLE->codec_to_negotiate;

				/* Let AGHFP know that the app has negotiated its codec. */
				AghfpClearAppCodecNegotiationPending(msg->aghfp);

				/* Let the app know that a codec has been negotiated (for querying via API). */
				SIMPLE->codec_negotiated = TRUE;

				/* Start the audio. */
				AghfpStartAudioAfterAppCodecNegotiation(msg->aghfp, SIMPLE->audio_params.syncPktTypes, &SIMPLE->audio_params.hfpAudioParams);
			}
		}
		else
		{
			if (csr2csrHandleFeatureNegotiationAgInitiated(msg->aghfp, msg->num_csr_features, codec, bandwidth, SIMPLE->codec_to_negotiate))
			{
				/* AG initiated CSR CODEC negotiation has now successfully completed. */
				/* Note and continue with SCO/eSCO. */
				SIMPLE->ag_initiated_at_csrfn_resolved = TRUE;
				SIMPLE->ag_initiated_csr_codec_negotiation = FALSE;

				/*store the codec*/
				SIMPLE->audio_codec = SIMPLE->codec_to_negotiate;

				/* Let AGHFP know that the app has negotiated its codec. */
				AghfpClearAppCodecNegotiationPending(msg->aghfp);

				/* Let the app know that a codec has been negotiated. */
				SIMPLE->codec_negotiated = TRUE;

				if (SIMPLE->start_audio_after_codec_negotiation)
				{
					SIMPLE->start_audio_after_codec_negotiation = FALSE;

					AghfpStartAudioAfterAppCodecNegotiation(msg->aghfp, SIMPLE->audio_params.syncPktTypes, &SIMPLE->audio_params.hfpAudioParams);
				}
			}
		}
	}
	break;
	case EventSysAGConnect:
		if (!SIMPLE->connected)
		{
			AghfpSlcConnect(SIMPLE->aghfp, &SIMPLE->bd_addr);
		}
		break;
	case EventSysAGDisconnect:
		if ((SIMPLE->connected) && (!SIMPLE->audio))
		{
			AghfpSlcDisconnect(SIMPLE->aghfp);
		}
		break;
	case EventSysAGAudioConnect:
	{
		if (SIMPLE->connected)
		{
			DEBUG(("toggle AUDIO [%x]\n", SIMPLE->audio));
			{
				DEBUG(("AUDIO CONNECT\n"));

				if (SIMPLE->audio_codec == audio_codec_cvsd)
				{
					DEBUG(("AUDIOPARAMS CVSD >ESCO: [%x] [%x]\n", SIMPLE->audio_cvsd_params.syncPktTypes, SIMPLE->audio_cvsd_params.hfpAudioParams.max_latency));
					AghfpAudioConnect(SIMPLE->aghfp, SIMPLE->audio_cvsd_params.syncPktTypes, &SIMPLE->audio_cvsd_params.hfpAudioParams);
				}
				else
				{
					DEBUG(("AUDIOPARAMS AURISTREAM >ESCO: [%x] [%x]\n", SIMPLE->audio_auristream_params.syncPktTypes, SIMPLE->audio_auristream_params.hfpAudioParams.max_latency));
					AghfpAudioConnect(SIMPLE->aghfp, SIMPLE->audio_auristream_params.syncPktTypes, &SIMPLE->audio_auristream_params.hfpAudioParams);
				}
			}
		}
	}
	break;
	case EventSysAGAudioDisconnect:
	{
		DEBUG(("AUDIO DISCONNECT\n"));
		AghfpAudioDisconnect(SIMPLE->aghfp);
	}
	break;
#if 0			
	case CODEC_INIT_CFM:
		{
			CODEC_INIT_CFM_T *msg = (CODEC_INIT_CFM_T*)message;
			SIMPLE->codec_task = msg->codecTask;

			read_config() ;
		 	
		}
	break ;

	case BUTTON_MFB_PRESS:
		DEBUG(("BUTTON_MFB_PRESS\n"));
		{
				/*only disconnect if connected without audio*/
			if ((SIMPLE->connected) && ( ! SIMPLE->audio) )
			{
				AghfpSlcDisconnect ( SIMPLE->aghfp) ;		
			}
			else
			{
				AghfpSlcConnect(SIMPLE->aghfp, &SIMPLE->bd_addr);
			}	
		}
	break ;
	case BUTTON_FCN_PRESS:
		{
			DEBUG(("BUTTON_FCN_PRESS[%x]\n" , SIMPLE->connected));		
			
            if (SIMPLE->connected)
            {
    			DEBUG(("toggle AUDIO [%x]\n" ,  SIMPLE->audio));	  
		      	if (SIMPLE->audio)
        		{
			     	DEBUG(("AUDIO DISCONNECT\n")) ;
	       			AghfpAudioDisconnect(SIMPLE->aghfp );                
    			}
			    else
			    {
	       			DEBUG(("AUDIO CONNECT\n")) ;
				
    				if (SIMPLE->audio_codec == audio_codec_cvsd)
			     	{
		  			DEBUG(("AUDIOPARAMS CVSD >ESCO: [%x] [%x]\n", SIMPLE->audio_cvsd_params.syncPktTypes , SIMPLE->audio_cvsd_params.hfpAudioParams.max_latency )) ;
	   				AghfpAudioConnect(SIMPLE->aghfp,  SIMPLE->audio_cvsd_params.syncPktTypes, &SIMPLE->audio_cvsd_params.hfpAudioParams );                
    			 	}
			     	else
	       			{
    					DEBUG(("AUDIOPARAMS AURISTREAM >ESCO: [%x] [%x]\n", SIMPLE->audio_auristream_params.syncPktTypes , SIMPLE->audio_auristream_params.hfpAudioParams.max_latency )) ;
					   AghfpAudioConnect(SIMPLE->aghfp, SIMPLE->audio_auristream_params.syncPktTypes, &SIMPLE->audio_auristream_params.hfpAudioParams );                
				    }
                }
            }
        }
	break ;
#endif
	default:
		DEBUG(("Main Unhandled Message : %d , 0x%0X\n", id, id));
		break;
	}
}

int AgInit(Task task)
{
	/*latch on the power*/

	SIMPLE = PanicUnlessMalloc(sizeof(SIMPLE_T));
	memset(SIMPLE, 0, sizeof(SIMPLE_T));
	/* Set up task handler */
	SIMPLE->task.handler = profile_handler;

	SIMPLE->app_task = task;

	SIMPLE->audio = FALSE;

	SIMPLE->audio_codec = audio_codec_cvsd;
	SIMPLE->codec_to_negotiate = audio_codec_cvsd;

	/* Set codec negotiation variables to sensible values. */
	SIMPLE->codec_to_negotiate = audio_codec_cvsd;
	SIMPLE->csr_codec_bandwidth_to_negotiate = auristream_bw_mask_8k;
	SIMPLE->negotiation_type = aghfp_codec_negotiation_type_none;
	SIMPLE->codec_negotiated = FALSE;
	SIMPLE->csr_codec_negotiation_capable = FALSE;
	SIMPLE->start_audio_after_codec_negotiation = FALSE;
	SIMPLE->hf_csr_codecs = 0;
	SIMPLE->hf_csr_codec_bandwidths = 0;
	SIMPLE->hf_supports_csr_bw_negotiation = 0;
	SIMPLE->hf_supports_csr_hf_initiated_codec_negotiation = 0;
	SIMPLE->volume = 0x0a;

	read_config();

	AghfpInit(&SIMPLE->task, aghfp_handsfree_15_profile, aghfp_incoming_call_reject | aghfp_inband_ring);
	return 0;
}

void AgConnect(bdaddr *addr)
{
	DEBUG(("AG: connect to bdaddr = %x:%x:%lx\n", addr->nap, addr->uap, addr->lap));
	AghfpSlcConnect(SIMPLE->aghfp, addr);
}

void AgDisconnect(void)
{
	MessageSend(&SIMPLE->task, EventSysAGDisconnect, NULL);
}

bool agVoicePopulateConnectParameters(audio_connect_parameters *connect_parameters)
{
		connect_parameters->app_task = SIMPLE->app_task;
        connect_parameters->audio_plugin = SIMPLE->audio_plugin;
        connect_parameters->audio_sink = SIMPLE->audio_sink;
        connect_parameters->features = sinkAudioGetPluginFeatures();
        connect_parameters->mode = AUDIO_MODE_CONNECTED;
        connect_parameters->params = sinkHfpDataGetHfpPluginParams();
        connect_parameters->power = powerManagerGetLBIPM();
        connect_parameters->rate = 8000;
        connect_parameters->route = AUDIO_ROUTE_INTERNAL;
        connect_parameters->sink_type = AUDIO_SINK_ESCO;
        connect_parameters->volume = 0xf;
        
        return TRUE;
}

Sink agGetActiveScoSink(void)
{
	return SIMPLE->audio_sink;
}

#endif
