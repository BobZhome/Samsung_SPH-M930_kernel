/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/mfd/msm-adie-codec.h>
#include <linux/uaccess.h>
#include <asm/mach-types.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/snddev_ecodec.h>
#include <mach/board.h>
#include <mach/qdsp5v2/snddev_icodec.h>
#include <mach/qdsp5v2/snddev_mi2s.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_acdb_def.h>
#include "timpani_profile_7x30.h"

/* define the value for BT_SCO */
#define BT_SCO_PCM_CTL_VAL (PCM_CTL__RPCM_WIDTH__LINEAR_V |\
		PCM_CTL__TPCM_WIDTH__LINEAR_V)
#define BT_SCO_DATA_FORMAT_PADDING (DATA_FORMAT_PADDING_INFO__RPCM_FORMAT_V |\
		DATA_FORMAT_PADDING_INFO__TPCM_FORMAT_V)
//#define BT_SCO_AUX_CODEC_INTF   AUX_CODEC_INTF_CTL__PCMINTF_DATA_EN_V
#define BT_SCO_AUX_CODEC_INTF (AUX_CODEC_INTF_CTL__PCMINTF_DATA_EN_V|\
                               AUX_CODEC_CTL__AUX_CODEC_MDOE__PCM_V|\
                               AUX_CODEC_CTL__AUX_PCM_MODE__AUX_MASTER_V) 


// CHIEF AUDIO DEVICE

// handset_rx
static struct adie_codec_action_unit handset_rx_48KHz_osr256_actions[] =
	HANDSET_RX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry handset_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(handset_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile handset_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = handset_rx_settings,
	.setting_sz = ARRAY_SIZE(handset_rx_settings),
};

static struct snddev_icodec_data snddev_handset_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_RX,
	.profile = &handset_rx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -1400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2900,
};

static struct platform_device msm_handset_rx_device = {
	.name = "snddev_icodec",
	.id = 0,
	.dev = { .platform_data = &snddev_handset_rx_data },
};


// handset_tx
static struct adie_codec_action_unit handset_tx_48KHz_osr256_actions[] =
	HANDSET_TX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry handset_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(handset_tx_48KHz_osr256_actions),
	}
};

static enum hsed_controller handset_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_dev_profile handset_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = handset_tx_settings,
	.setting_sz = ARRAY_SIZE(handset_tx_settings),
};

static struct snddev_icodec_data snddev_handset_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_TX,
	.profile = &handset_tx_profile,
	.channel_mode = 1,
	.pmctl_id = handset_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_handset_tx_device = {
	.name = "snddev_icodec",
	.id = 1,
	.dev = { .platform_data = &snddev_handset_tx_data },
};

// speaker_rx
static struct adie_codec_action_unit speaker_rx_48KHz_osr256_actions[] =
	SPEAKER_RX_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(speaker_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile speaker_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = speaker_rx_settings,
	.setting_sz = ARRAY_SIZE(speaker_rx_settings),
};

static struct snddev_icodec_data snddev_speaker_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_RX,
	.profile = &speaker_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_speaker,
	.pamp_off = msm_snddev_poweramp_off_speaker,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700
};

static struct platform_device msm_speaker_rx_device = {
	.name = "snddev_icodec",
	.id = 2,
	.dev = { .platform_data = &snddev_speaker_rx_data },
};

// speaker_tx
static enum hsed_controller speaker_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_action_unit speaker_tx_osr256_actions[] =
	SPEAKER_TX_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(speaker_tx_osr256_actions),
	}
};


static struct adie_codec_dev_profile speaker_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = speaker_tx_settings,
	.setting_sz = ARRAY_SIZE(speaker_tx_settings),
};

static struct snddev_icodec_data snddev_speaker_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_TX,
	.profile = &speaker_tx_profile,
	.channel_mode = 1,
	.pmctl_id = speaker_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(speaker_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_speaker_tx_device = {
	.name = "snddev_icodec",
	.id = 3,
	.dev = { .platform_data = &snddev_speaker_tx_data },
};


// headset_rx

static struct adie_codec_action_unit headset_rx_48KHz_osr256_actions[] =
	HEADSET_RX_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_rx_settings,
	.setting_sz = ARRAY_SIZE(headset_rx_settings),
};

static struct snddev_icodec_data snddev_headset_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_RX,
	.profile = &headset_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_headset,
	.pamp_off = msm_snddev_poweramp_off_headset,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -1400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2900,
};

static struct platform_device msm_headset_rx_device = {
	.name = "snddev_icodec",
	.id = 4,
	.dev = { .platform_data = &snddev_headset_rx_data },
};


// headset_tx
static struct adie_codec_action_unit headset_tx_actions[] =
	HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry headset_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_tx_actions,
		.action_sz = ARRAY_SIZE(headset_tx_actions),
	},
};

static struct adie_codec_dev_profile headset_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = headset_tx_settings,
	.setting_sz = ARRAY_SIZE(headset_tx_settings),
};

static struct snddev_icodec_data snddev_headset_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_TX,
	.profile = &headset_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_headset_tx_device = {
	.name = "snddev_icodec",
	.id = 5,
	.dev = { .platform_data = &snddev_headset_tx_data },
};



// bt sco rx

static struct snddev_ecodec_data snddev_bt_sco_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_RX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
};

static struct platform_device msm_bt_sco_rx_device = {
	.name = "msm_snddev_ecodec",
	.id = 6,
	.dev = { .platform_data = &snddev_bt_sco_rx_data },
};

// bt sco tx
static struct snddev_ecodec_data snddev_bt_sco_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_TX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

static struct platform_device msm_bt_sco_tx_device = {
	.name = "msm_snddev_ecodec",
	.id = 7,
	.dev = { .platform_data = &snddev_bt_sco_tx_data },
};

// bt sco nrec rx

static struct snddev_ecodec_data snddev_bt_sco_nrec_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_nrec_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_NREC_RX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
};

static struct platform_device msm_bt_sco_nrec_rx_device = {
	.name = "msm_snddev_ecodec",
	.id = 8,
	.dev = { .platform_data = &snddev_bt_sco_nrec_rx_data },
};

// bt sco tx
static struct snddev_ecodec_data snddev_bt_sco_nrec_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_nrec_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_NREC_TX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

static struct platform_device msm_bt_sco_nrec_tx_device = {
	.name = "msm_snddev_ecodec",
	.id = 9,
	.dev = { .platform_data = &snddev_bt_sco_nrec_tx_data },
};

// aux_dock_rx  :: the use of speaker rx path 
static struct adie_codec_action_unit aux_dock_rx_48KHz_osr256_actions[] =
	AUX_DOCK_RX_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry aux_dock_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = aux_dock_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(aux_dock_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile aux_dock_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = aux_dock_rx_settings,
	.setting_sz = ARRAY_SIZE(aux_dock_rx_settings),
};

static struct snddev_icodec_data snddev_aux_dock_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "aux_dock_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_AUX_DOCK_RX,
	.profile = &aux_dock_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_aux_dock_rx_route_config,
	.pamp_off = msm_snddev_aux_dock_rx_route_deconfig,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700
};

static struct platform_device msm_aux_dock_rx_device = {
	.name = "snddev_icodec",
	.id = 11,
	.dev = { .platform_data = &snddev_aux_dock_rx_data },
};


// speaker_headset_rx
static struct adie_codec_action_unit speaker_headset_rx_48KHz_osr256_actions[] =
	SPEAKER_HPH_AB_CPL_PRI_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_headset_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_headset_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(speaker_headset_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile speaker_headset_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = speaker_headset_rx_settings,
	.setting_sz = ARRAY_SIZE(speaker_headset_rx_settings),
};

static struct snddev_icodec_data snddev_speaker_headset_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_headset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_HEADSET_RX,
	.profile = &speaker_headset_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_together,
	.pamp_off = msm_snddev_poweramp_off_together,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700
};

static struct platform_device msm_speaker_headset_rx_device = {
	.name = "snddev_icodec",
	.id = 12,
	.dev = { .platform_data = &snddev_speaker_headset_rx_data },
};



// handset_rx
static struct adie_codec_action_unit handset_call_rx_48KHz_osr256_actions[] =
	HANDSET_CALL_RX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry handset_call_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_call_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(handset_call_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile handset_call_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = handset_call_rx_settings,
	.setting_sz = ARRAY_SIZE(handset_call_rx_settings),
};

static struct snddev_icodec_data snddev_handset_call_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_call_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_CALL_RX,
	.profile = &handset_call_rx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700,
};

static struct platform_device msm_handset_call_rx_device = {
	.name = "snddev_icodec",
	.id = 15,
	.dev = { .platform_data = &snddev_handset_call_rx_data },
};


// handset_call_tx
static struct adie_codec_action_unit handset_call_tx_48KHz_osr256_actions[] =
	HANDSET_CALL_TX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry handset_call_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_call_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(handset_call_tx_48KHz_osr256_actions),
	}
};

static enum hsed_controller handset_call_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_dev_profile handset_call_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = handset_call_tx_settings,
	.setting_sz = ARRAY_SIZE(handset_call_tx_settings),
};

static struct snddev_icodec_data snddev_handset_call_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_call_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_CALL_TX,
	.profile = &handset_call_tx_profile,
	.channel_mode = 1,
	.pmctl_id = handset_call_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_call_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_handset_call_tx_device = {
	.name = "snddev_icodec",
	.id = 16,
	.dev = { .platform_data = &snddev_handset_call_tx_data },
};

// speaker_call_rx
static struct adie_codec_action_unit speaker_call_rx_48KHz_osr256_actions[] =
	SPEAKER_CALL_RX_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_call_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_call_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(speaker_call_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile speaker_call_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = speaker_call_rx_settings,
	.setting_sz = ARRAY_SIZE(speaker_call_rx_settings),
};

static struct snddev_icodec_data snddev_speaker_call_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_call_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_CALL_RX,
	.profile = &speaker_call_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_speaker_incall,
	.pamp_off = msm_snddev_poweramp_off_speaker_incall,
	.max_voice_rx_vol[VOC_NB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_NB_INDEX] = -500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_WB_INDEX] = -500
};

static struct platform_device msm_speaker_call_rx_device = {
	.name = "snddev_icodec",
	.id = 17,
	.dev = { .platform_data = &snddev_speaker_call_rx_data },
};

// speaker_call_tx
static enum hsed_controller speaker_call_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_action_unit speaker_call_tx_osr256_actions[] =
	SPEAKER_CALL_TX_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_call_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_call_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(speaker_call_tx_osr256_actions),
	}
};


static struct adie_codec_dev_profile speaker_call_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = speaker_call_tx_settings,
	.setting_sz = ARRAY_SIZE(speaker_call_tx_settings),
};

static struct snddev_icodec_data snddev_speaker_call_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_call_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_CALL_TX,
	.profile = &speaker_call_tx_profile,
	.channel_mode = 1,
	.pmctl_id = speaker_call_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(speaker_call_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_speaker_call_tx_device = {
	.name = "snddev_icodec",
	.id = 18,
	.dev = { .platform_data = &snddev_speaker_call_tx_data },
};


// headset_call_rx

static struct adie_codec_action_unit headset_call_rx_48KHz_osr256_actions[] =
	HEADSET_CALL_RX_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_call_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_call_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_call_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_call_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_call_rx_settings,
	.setting_sz = ARRAY_SIZE(headset_call_rx_settings),
};

static struct snddev_icodec_data snddev_headset_call_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_call_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_CALL_RX,
	.profile = &headset_call_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_headset,
	.pamp_off = msm_snddev_poweramp_off_headset,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -700,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2200,
};

static struct platform_device msm_headset_call_rx_device = {
	.name = "snddev_icodec",
	.id = 19,
	.dev = { .platform_data = &snddev_headset_call_rx_data },
};


// headset_tx
static struct adie_codec_action_unit headset_call_tx_actions[] =
	HEADSET_CALL_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry headset_call_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_call_tx_actions,
		.action_sz = ARRAY_SIZE(headset_call_tx_actions),
	},
};

static struct adie_codec_dev_profile headset_call_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = headset_call_tx_settings,
	.setting_sz = ARRAY_SIZE(headset_call_tx_settings),
};

static struct snddev_icodec_data snddev_headset_call_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_call_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_CALL_TX,
	.profile = &headset_call_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_headset_call_tx_device = {
	.name = "snddev_icodec",
	.id = 20,
	.dev = { .platform_data = &snddev_headset_call_tx_data },
};



// bt sco rx

static struct snddev_ecodec_data snddev_bt_sco_call_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_call_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_CALL_RX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
};

static struct platform_device msm_bt_sco_call_rx_device = {
	.name = "msm_snddev_ecodec",
	.id = 21,
	.dev = { .platform_data = &snddev_bt_sco_call_rx_data },
};

// bt sco tx
static struct snddev_ecodec_data snddev_bt_sco_call_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_call_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_CALL_TX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

static struct platform_device msm_bt_sco_call_tx_device = {
	.name = "msm_snddev_ecodec",
	.id = 22,
	.dev = { .platform_data = &snddev_bt_sco_call_tx_data },
};

// bt sco nrec rx

static struct snddev_ecodec_data snddev_bt_sco_nrec_call_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_nrec_call_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_NREC_CALL_RX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
};

static struct platform_device msm_bt_sco_nrec_call_rx_device = {
	.name = "msm_snddev_ecodec",
	.id = 23,
	.dev = { .platform_data = &snddev_bt_sco_nrec_call_rx_data },
};

// bt sco nrec tx
static struct snddev_ecodec_data snddev_bt_sco_nrec_call_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_nrec_call_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_NREC_CALL_TX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

static struct platform_device msm_bt_sco_nrec_call_tx_device = {
	.name = "msm_snddev_ecodec",
	.id = 24,
	.dev = { .platform_data = &snddev_bt_sco_nrec_call_tx_data },
};

// handset_call_dualmic_tx
static struct adie_codec_action_unit handset_call_dualmic_tx_48KHz_osr256_actions[] =
	HANDSET_CALL_DUALMIC_TX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry handset_call_dualmic_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_call_dualmic_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(handset_call_dualmic_tx_48KHz_osr256_actions),
	}
};

static enum hsed_controller handset_call_dualmic_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_dev_profile handset_call_dualmic_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = handset_call_dualmic_tx_settings,
	.setting_sz = ARRAY_SIZE(handset_call_dualmic_tx_settings),
};

static struct snddev_icodec_data snddev_handset_call_dualmic_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_call_dualmic_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_DUALMIC_HANDSET_CALL_TX,
	.profile = &handset_call_dualmic_tx_profile,
	.channel_mode = 1,
	.pmctl_id = handset_call_dualmic_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_call_dualmic_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_handset_call_dualmic_tx_device = {
	.name = "snddev_icodec",
	.id = 27,
	.dev = { .platform_data = &snddev_handset_call_dualmic_tx_data },
};

// fmradio_handset_rx
static struct adie_codec_action_unit fmradio_handset_rx_48KHz_osr256_actions[] =
	FMRADIO_HANDSET_RX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry fmradio_handset_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = fmradio_handset_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(fmradio_handset_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile fmradio_handset_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = fmradio_handset_rx_settings,
	.setting_sz = ARRAY_SIZE(fmradio_handset_rx_settings),
};

static struct snddev_icodec_data snddev_fmradio_handset_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "fmradio_handset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_FMRADIO_RX,
	.profile = &fmradio_handset_rx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -1400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2900,
};

static struct platform_device msm_fmradio_handset_rx_device = {
	.name = "snddev_icodec",
	.id = 28,
	.dev = { .platform_data = &snddev_fmradio_handset_rx_data },
};

// fmradio_headset_rx

static struct adie_codec_action_unit fmradio_headset_rx_48KHz_osr256_actions[] =
	FMRADIO_HEADSET_RX_48000_OSR_256;

static struct adie_codec_hwsetting_entry fmradio_headset_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = fmradio_headset_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(fmradio_headset_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile fmradio_headset_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = fmradio_headset_rx_settings,
	.setting_sz = ARRAY_SIZE(fmradio_headset_rx_settings),
};

static struct snddev_icodec_data snddev_fmradio_headset_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "fmradio_headset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_FMRADIO_RX,
	.profile = &fmradio_headset_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_headset,
	.pamp_off = msm_snddev_poweramp_off_headset,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -1400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2900,
};

static struct platform_device msm_fmradio_headset_rx_device = {
	.name = "snddev_icodec",
	.id = 29,
	.dev = { .platform_data = &snddev_fmradio_headset_rx_data },
};


// fmradio_headset_tx
static struct adie_codec_action_unit fmradio_headset_tx_actions[] =
	FMRADIO_HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry fmradio_headset_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = fmradio_headset_tx_actions,
		.action_sz = ARRAY_SIZE(fmradio_headset_tx_actions),
	},
};

static struct adie_codec_dev_profile fmradio_headset_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = fmradio_headset_tx_settings,
	.setting_sz = ARRAY_SIZE(fmradio_headset_tx_settings),
};

static struct snddev_icodec_data snddev_fmradio_headset_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "fmradio_headset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_FMRADIO_TX,
	.profile = &fmradio_headset_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_fmradio_headset_tx_device = {
	.name = "snddev_icodec",
	.id = 30,
	.dev = { .platform_data = &snddev_fmradio_headset_tx_data },
};


// fmradio_speaker_rx
static struct adie_codec_action_unit fmradio_speaker_rx_48KHz_osr256_actions[] =
	FMRADIO_SPEAKER_RX_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry fmradio_speaker_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = fmradio_speaker_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(fmradio_speaker_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile fmradio_speaker_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = fmradio_speaker_rx_settings,
	.setting_sz = ARRAY_SIZE(fmradio_speaker_rx_settings),
};

static struct snddev_icodec_data snddev_fmradio_speaker_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "fmradio_speaker_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_FMRADIO_RX,
	.profile = &fmradio_speaker_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_speaker,
	.pamp_off = msm_snddev_poweramp_off_speaker,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700
};

static struct platform_device msm_fmradio_speaker_rx_device = {
	.name = "snddev_icodec",
	.id = 31,
	.dev = { .platform_data = &snddev_fmradio_speaker_rx_data },
};

// voicedialer_speaker_tx
static enum hsed_controller voicedialer_speaker_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_action_unit voicedialer_speaker_tx_osr256_actions[] =
	VOICEDIALER_SPEAKER_TX_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry voicedialer_speaker_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = voicedialer_speaker_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(voicedialer_speaker_tx_osr256_actions),
	}
};


static struct adie_codec_dev_profile voicedialer_speaker_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = voicedialer_speaker_tx_settings,
	.setting_sz = ARRAY_SIZE(voicedialer_speaker_tx_settings),
};

static struct snddev_icodec_data snddev_voicedialer_speaker_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "voicedialer_speaker_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC,
	.profile = &voicedialer_speaker_tx_profile,
	.channel_mode = 1,
	.pmctl_id = voicedialer_speaker_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(voicedialer_speaker_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_voicedialer_speaker_tx_device = {
	.name = "snddev_icodec",
	.id = 32,
	.dev = { .platform_data = &snddev_voicedialer_speaker_tx_data },
};


// voicedialer_headset_tx
static struct adie_codec_action_unit voicedialer_headset_tx_actions[] =
	VOICEDIALER_HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry voicedialer_headset_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = voicedialer_headset_tx_actions,
		.action_sz = ARRAY_SIZE(voicedialer_headset_tx_actions),
	},
};

static struct adie_codec_dev_profile voicedialer_headset_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = voicedialer_headset_tx_settings,
	.setting_sz = ARRAY_SIZE(voicedialer_headset_tx_settings),
};

static struct snddev_icodec_data snddev_voicedialer_headset_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "voicedialer_headset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_VOICE_DIALER_TX,
	.profile = &voicedialer_headset_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_voicedialer_headset_tx_device = {
	.name = "snddev_icodec",
	.id = 33,
	.dev = { .platform_data = &snddev_voicedialer_headset_tx_data },
};


// voice dialer bt sco tx
static struct snddev_ecodec_data snddev_voicedialer_bt_sco_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "voicedialer_bt_sco_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_VOICE_DIALER_TX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

static struct platform_device msm_voicedialer_bt_sco_tx_device = {
	.name = "msm_snddev_ecodec",
	.id = 34,
	.dev = { .platform_data = &snddev_voicedialer_bt_sco_tx_data },
};

// voice dialer bt sco tx nrec
static struct snddev_ecodec_data snddev_voicedialer_bt_sco_nrec_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "voicedialer_bt_sco_nrec_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_NREC_VOICE_DIALER_TX,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

static struct platform_device msm_voicedialer_bt_sco_nrec_tx_device = {
	.name = "msm_snddev_ecodec",
	.id = 35,
	.dev = { .platform_data = &snddev_voicedialer_bt_sco_nrec_tx_data },
};


// voicesearch_speaker_tx
static enum hsed_controller voicesearch_speaker_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_action_unit voicesearch_speaker_tx_osr256_actions[] =
	VOICEDIALER_SPEAKER_TX_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry voicesearch_speaker_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = voicesearch_speaker_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(voicesearch_speaker_tx_osr256_actions),
	}
};


static struct adie_codec_dev_profile voicesearch_speaker_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = voicesearch_speaker_tx_settings,
	.setting_sz = ARRAY_SIZE(voicesearch_speaker_tx_settings),
};

static struct snddev_icodec_data snddev_voicesearch_speaker_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_voice_search_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_VOICE_SEARCH_TX,
	.profile = &voicesearch_speaker_tx_profile,
	.channel_mode = 1,
	.pmctl_id = voicesearch_speaker_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(voicesearch_speaker_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_voicesearch_speaker_tx_device = {
	.name = "snddev_icodec",
	.id = 36,
	.dev = { .platform_data = &snddev_voicesearch_speaker_tx_data },
};


// voicesearch_headset_tx
static struct adie_codec_action_unit voicesearch_headset_tx_actions[] =
	VOICEDIALER_HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry voicesearch_headset_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = voicesearch_headset_tx_actions,
		.action_sz = ARRAY_SIZE(voicesearch_headset_tx_actions),
	},
};

static struct adie_codec_dev_profile voicesearch_headset_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = voicesearch_headset_tx_settings,
	.setting_sz = ARRAY_SIZE(voicesearch_headset_tx_settings),
};

static struct snddev_icodec_data snddev_voicesearch_headset_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_voice_search_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_VOICE_SEARCH_TX,
	.profile = &voicesearch_headset_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_voicesearch_headset_tx_device = {
	.name = "snddev_icodec",
	.id = 37,
	.dev = { .platform_data = &snddev_voicesearch_headset_tx_data },
};



// tty rx
static struct adie_codec_action_unit itty_mono_rx_actions[] =
	TTY_HEADSET_MONO_RX_8000_OSR_256;

static struct adie_codec_hwsetting_entry itty_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_mono_rx_actions,
		.action_sz = ARRAY_SIZE(itty_mono_rx_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = itty_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_rx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_call_rx",   //"tty_headset_mono_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_MONO_CALL_RX,
	.profile = &itty_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_tty,    //NULL,
	.pamp_off = msm_snddev_poweramp_off_tty,  //NULL,
	.max_voice_rx_vol[VOC_NB_INDEX] = -2000,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2000,
	.max_voice_rx_vol[VOC_WB_INDEX] = -2000,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2000,
};

static struct platform_device msm_itty_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 25,
	.dev = { .platform_data = &snddev_itty_mono_rx_data },
};


// tty tx

static struct adie_codec_action_unit itty_mono_tx_actions[] =
	TTY_HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry itty_mono_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_mono_tx_actions,
		.action_sz = ARRAY_SIZE(itty_mono_tx_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = itty_mono_tx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_tx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_call_tx",   //"tty_headset_mono_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_MONO_CALL_TX,
	.profile = &itty_mono_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_itty_mono_tx_device = {
	.name = "snddev_icodec",
	.id = 26,
	.dev = { .platform_data = &snddev_itty_mono_tx_data },
};

// headset_loopback_tx
static struct adie_codec_action_unit headset_loopback_tx_actions[] =
	HEADSET_LOOPBACK_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry headset_loopback_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_loopback_tx_actions,
		.action_sz = ARRAY_SIZE(headset_loopback_tx_actions),
	},
};

static struct adie_codec_dev_profile headset_loopback_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = headset_loopback_tx_settings,
	.setting_sz = ARRAY_SIZE(headset_loopback_tx_settings),
};

static struct snddev_icodec_data snddev_headset_loopback_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_loopback_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_CALL_TX,
	.profile = &headset_loopback_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_headset_loopback_tx_device = {
	.name = "snddev_icodec",
	.id = 38,
	.dev = { .platform_data = &snddev_headset_loopback_tx_data },
};

// handset_voip_rx

//static struct adie_codec_action_unit handset_rx_48KHz_osr256_actions[] =
//	HANDSET_RX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry handset_voip_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(handset_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile handset_voip_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = handset_voip_rx_settings,
	.setting_sz = ARRAY_SIZE(handset_voip_rx_settings),
};

static struct snddev_icodec_data snddev_handset_voip_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_voip_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_VOIP_RX,
	.profile = &handset_voip_rx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -1400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2900,
};

static struct platform_device msm_handset_voip_rx_device = {
	.name = "snddev_icodec",
	.id = 39,
	.dev = { .platform_data = &snddev_handset_voip_rx_data },
};


// handset_voip_tx
//static struct adie_codec_action_unit handset_tx_48KHz_osr256_actions[] =
//	HANDSET_TX_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry handset_voip_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_tx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(handset_tx_48KHz_osr256_actions),
	}
};

//static enum hsed_controller handset_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_dev_profile handset_voip_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = handset_voip_tx_settings,
	.setting_sz = ARRAY_SIZE(handset_voip_tx_settings),
};

static struct snddev_icodec_data snddev_handset_voip_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_voip_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_VOIP_TX,
	.profile = &handset_voip_tx_profile,
	.channel_mode = 1,
	.pmctl_id = handset_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_handset_voip_tx_device = {
	.name = "snddev_icodec",
	.id = 40,
	.dev = { .platform_data = &snddev_handset_voip_tx_data },
};

// speaker_voip_rx
//static struct adie_codec_action_unit speaker_rx_48KHz_osr256_actions[] =
//	SPEAKER_RX_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_voip_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(speaker_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile speaker_voip_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = speaker_voip_rx_settings,
	.setting_sz = ARRAY_SIZE(speaker_voip_rx_settings),
};

static struct snddev_icodec_data snddev_speaker_voip_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_voip_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_VOIP_RX,
	.profile = &speaker_voip_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_speaker_inVoip,
	.pamp_off = msm_snddev_poweramp_off_speaker_inVoip,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700
};

static struct platform_device msm_speaker_voip_rx_device = {
	.name = "snddev_icodec",
	.id = 41,
	.dev = { .platform_data = &snddev_speaker_voip_rx_data },
};

// speaker_voip_tx
//static enum hsed_controller speaker_tx_pmctl_id[] = {PM_HSED_CONTROLLER_0};

//static struct adie_codec_action_unit speaker_tx_osr256_actions[] =
//	SPEAKER_TX_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_voip_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(speaker_tx_osr256_actions),
	}
};


static struct adie_codec_dev_profile speaker_voip_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = speaker_voip_tx_settings,
	.setting_sz = ARRAY_SIZE(speaker_voip_tx_settings),
};

static struct snddev_icodec_data snddev_speaker_voip_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_voip_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPEAKER_VOIP_TX,
	.profile = &speaker_voip_tx_profile,
	.channel_mode = 1,
	.pmctl_id = speaker_tx_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(speaker_tx_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_speaker_voip_tx_device = {
	.name = "snddev_icodec",
	.id = 42,
	.dev = { .platform_data = &snddev_speaker_voip_tx_data },
};


// headset_voip_rx

//static struct adie_codec_action_unit headset_rx_48KHz_osr256_actions[] =
//	HEADSET_RX_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_voip_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_voip_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_voip_rx_settings,
	.setting_sz = ARRAY_SIZE(headset_voip_rx_settings),
};

static struct snddev_icodec_data snddev_headset_voip_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_voip_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_VOIP_RX,
	.profile = &headset_voip_rx_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_headset,
	.pamp_off = msm_snddev_poweramp_off_headset,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -1400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2900,
};

static struct platform_device msm_headset_voip_rx_device = {
	.name = "snddev_icodec",
	.id = 43,
	.dev = { .platform_data = &snddev_headset_voip_rx_data },
};


// headset_voip_tx
//static struct adie_codec_action_unit headset_tx_actions[] =
//	HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry headset_voip_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_tx_actions,
		.action_sz = ARRAY_SIZE(headset_tx_actions),
	},
};

static struct adie_codec_dev_profile headset_voip_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = headset_voip_tx_settings,
	.setting_sz = ARRAY_SIZE(headset_voip_tx_settings),
};

static struct snddev_icodec_data snddev_headset_voip_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_voip_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_VOIP_TX,
	.profile = &headset_voip_tx_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_tx_route_config,
	.pamp_off = msm_snddev_headset_tx_route_deconfig,
};

static struct platform_device msm_headset_voip_tx_device = {
	.name = "snddev_icodec",
	.id = 44,
	.dev = { .platform_data = &snddev_headset_voip_tx_data },
};




static struct platform_device *snd_devices_chief[] __initdata = {
    &msm_handset_rx_device,
    &msm_handset_tx_device,
    &msm_speaker_rx_device,
    &msm_speaker_tx_device,
    &msm_headset_rx_device,
    &msm_headset_tx_device,
    &msm_bt_sco_rx_device,	
    &msm_bt_sco_tx_device,
    &msm_bt_sco_nrec_rx_device,	
    &msm_bt_sco_nrec_tx_device, 
    //	&msm_hdmi_stereo_rx,
    &msm_aux_dock_rx_device,    
    &msm_speaker_headset_rx_device, 
    //	&msm_speaker_dock_rx,
    //	&msm_speaker_hdmi_rx,    
    &msm_handset_call_rx_device,
    &msm_handset_call_tx_device,
    &msm_speaker_call_rx_device,
    &msm_speaker_call_tx_device,
    &msm_headset_call_rx_device,
    &msm_headset_call_tx_device,
    &msm_bt_sco_call_rx_device,	
    &msm_bt_sco_call_tx_device,	
    &msm_bt_sco_nrec_call_rx_device,	
    &msm_bt_sco_nrec_call_tx_device,  
    &msm_itty_mono_rx_device,
    &msm_itty_mono_tx_device,  
    &msm_handset_call_dualmic_tx_device,
    &msm_fmradio_handset_rx_device,
    &msm_fmradio_headset_rx_device,
    &msm_fmradio_headset_tx_device,    
    &msm_fmradio_speaker_rx_device,	  
    &msm_voicedialer_speaker_tx_device,
    &msm_voicedialer_headset_tx_device,
    &msm_voicedialer_bt_sco_tx_device,
    &msm_voicedialer_bt_sco_nrec_tx_device,	
    &msm_voicesearch_speaker_tx_device,
    &msm_voicesearch_headset_tx_device,	
    &msm_headset_loopback_tx_device,
    &msm_handset_voip_rx_device,
    &msm_handset_voip_tx_device,
    &msm_speaker_voip_rx_device,
    &msm_speaker_voip_tx_device,
    &msm_headset_voip_rx_device,
    &msm_headset_voip_tx_device,

};

void __ref msm_snddev_init_timpani(void)
{
	platform_add_devices(snd_devices_chief,
			ARRAY_SIZE(snd_devices_chief));
}
