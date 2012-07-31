/****************************************************************************
 *
 *		CONFIDENTIAL
 *
 *		Copyright (c) 2011 Yamaha Corporation
 *
 *		Module		: D4SS1_Ctrl.h
 *
 *		Description	: D-4SS1 control module define
 *
 *		Version		: 1.0.0 	2011.11.24
 *
 ****************************************************************************/
#ifndef __LINUX_I2C_YDA173_H
#define __LINUX_I2C_YDA173_H

/* user setting */
/****************************************************************/
#define HP_HIZ_ON				/* HP Hi-Z is on */
/* #define SP_HIZ_ON */			/* SP Hi-Z is on */

#define D4SS1_OFFSEQUENCE_WAITTIME	30	/* "power off" sequence wait time [msec] */
/****************************************************************/

/* user implementation function */
/****************************************************************/
signed long d4Write(unsigned char bWriteRN, unsigned char bWritePrm);									/* write register function */
signed long d4WriteN(unsigned char bWriteRN, unsigned char *pbWritePrm, unsigned char dWriteSize);	/* write register function */
signed long d4Read(unsigned char bReadRN, unsigned char *pbReadPrm);		/* read register function */
void d4Wait(unsigned long dTime);				/* wait function */
void d4Sleep(unsigned long dTime);			/* sleep function */
void d4ErrHandler(signed long dError);		/* error handler function */
/****************************************************************/

/********************************************************************************************/
/* #0 */
#define D4SS1_SRST		0x808007
#define D4SS1_CT_CAN		0x806005
#define D4SS1_PPDOFF		0x801004
#define D4SS1_MODESEL		0x800803
#define D4SS1_CPMODE		0x800402
#define D4SS1_VLEVEL		0x800100

/* #1 */
#define D4SS1_DRC_MODE		0x81C006
#define D4SS1_DATRT		0x813004
#define D4SS1_NG_ATRT		0x810C02

/* #2 */
#define D4SS1_DPLT		0x82E005
#define D4SS1_HP_NG_RAT		0x821C02
#define D4SS1_HP_NG_ATT		0x820300

/* #3 */
#define D4SS1_DPLT_EX		0x834006
#define D4SS1_NCLIP		0x832005
#define D4SS1_SP_NG_RAT		0x831C02
#define D4SS1_SP_NG_ATT		0x830300

/* #4 */
#define D4SS1_VA		0x84F004
#define D4SS1_VB		0x840F00

/* #5 */
#define D4SS1_DIFA		0x858007
#define D4SS1_DIFB		0x854006
#define D4SS1_HP_SVOFF		0x850803
#define D4SS1_HP_HIZ		0x850402
#define D4SS1_SP_SVOFF		0x850201
#define D4SS1_SP_HIZ		0x850100

/* #6 */
#define D4SS1_VOLMODE		0x868007
#define D4SS1_SP_ATT		0x867F00

/* #7 */
#define D4SS1_HP_ATT		0x877F00

/* #8 */
#define D4SS1_OCP_ERR		0x888007
#define D4SS1_OTP_ERR		0x884006
#define D4SS1_DC_ERR		0x882005
#define D4SS1_HP_MONO		0x881004
#define D4SS1_HP_AMIX		0x880803
#define D4SS1_HP_BMIX		0x880402
#define D4SS1_SP_AMIX		0x880201
#define D4SS1_SP_BMIX		0x880100

/********************************************************************************************/

/* return value */
#define D4SS1_SUCCESS			0
#define D4SS1_ERROR				-1
#define D4SS1_ERROR_ARGUMENT	-2

/* D-4HP3 value */
#define D4SS1_MIN_REGISTERADDRESS			0x80
#define D4SS1_MAX_WRITE_REGISTERADDRESS		0x88
#define D4SS1_MAX_READ_REGISTERADDRESS		0x88

/* type */
#define SINT32 signed long
#define UINT32 unsigned long
#define SINT8 signed char
#define UINT8 unsigned char

/* structure */
/********************************************************************************************/
/* D-4HP3 setting information */
typedef struct {
	/* input */
	unsigned char bLine1Gain;		/* LINE1 Gain Amp D4SS1_VA	*/	
	unsigned char bLine2Gain;		/* LINE2 Gain Amp D4SS1_VB	*/	

	unsigned char bLine1Balance;	/* LINE1 Single-ended(0) or Differential(1) D4SS1_DIFA*/
	unsigned char bLine2Balance;	/* LINE2 Single-ended(0) or Differential(1) D4SS1_DIFB*/

	unsigned char bLineVolMode;		/* How input are use  D4SS1_VOLMODE		*/
	/*HP*/
	unsigned char bHpCtCancel;			/* HP CrossTalk Cancel D4SS1_CT_CAN*/
	unsigned char bHpPpdOff;			/* HP partial powerdownfunction D4SS1_PPDOFF*/
	unsigned char bHpModeSel;			/* HP enable sound output within 10ms D4SS1_MODESEL*/		
	unsigned char bHpCpMode;			/* HP charge pump mode setting, 3stage mode(0) / 2stage mode(1) D4SS1_CPMODE*/		
	unsigned char bHpAvddLev;			/* HP charge pump AVDD level, 1.65V<=AVDD<2.40V(0) / 2.40V<=AVDD<=2.86V(1) D4SS1_VLEVEL*/		

	unsigned char bHpNg_AttackTime;		/* HP Noise Gate : attack time D4SS1_NG_ATRT*/

	unsigned char bHpNg_Rat; 			/* HP Detection Level of headphone Amplifier's Noise GateD4SS1_HP_NG_RAT	*/	
	unsigned char bHpNg_Att; 			/* HP Attenuation Level for headphone Amplifier's Noise GateD4SS1_HP_NG_ATT	*/	

	unsigned char bHpSvol;				/* HP soft volume setting, on(0) / off(1) D4SS1_HP_SVOFF		*/
	unsigned char bHpHiZ;				/* HP Hiz state D4SS1_HP_HIZ*/

	unsigned char bHpAtt;				/* HP attenuator D4SS1_HP_ATT		*/
			
	unsigned char bHpCh;				/* HP channel, stereo(0)/mono(1) D4SS1_HP_MONO		*/
	unsigned char bHpMixer_Line1;		/* HP mixer LINE1 setting D4SS1_HP_AMIX		*/
	unsigned char bHpMixer_Line2;		/* HP mixer LINE2 setting D4SS1_HP_BMIX		*/

	/*SP*/
	unsigned char bSpModeSel;			/* SP enable sound output within 10ms D4SS1_MODESEL		*/
	
	unsigned char bSpDrcMode;			/* SP DRC or Non-Clip S function D4SS1_DRC_MODE		*/
	unsigned char bSpNcpl_AttackTime;	/* SP Non-Clip power limiter : attack Time D4SS1_DATRT		*/
	unsigned char bSpNcpl_ReleaseTime;	/* SP Non-Clip power limiter : release Time D4SS1_DATRT		*/
	unsigned char bSpNg_AttackTime;		/* SP Noise Gate : attack time D4SS1_NG_ATRT		*/

	unsigned char bSpNcpl_PowerLimit;	/* SP Non-Clip power limiter : Power Limit D4SS1_DPLT		*/

	unsigned char bSpNcpl_PowerLimitEx;	/* SP Non-Clip power limiter : Power Limit ExD4SS1_DPLT_EX		*/
	unsigned char bSpNclip;				/* SP On/Off Non-Clip funtion D4SS1_NCLIP		*/
	unsigned char bSpNg_Rat; 			/* SP Detection Level of headphone Amplifier's Noise GateD4SS1_SP_NG_RAT	*/	
	unsigned char bSpNg_Att; 			/* SP Attenuation Level for headphone Amplifier's Noise GateD4SS1_SP_NG_ATT	*/	

	unsigned char bSpSvol;				/* SP soft volume setting, on(0) / off(1) D4SS1_SP_SVOFF		*/
	unsigned char bSpHiZ;				/* SP HiZ state D4SS1_SP_HIZ	*/

	unsigned char bSpAtt;				/* SP attenuator D4SS1_SP_ATT	*/

	unsigned char bSpMixer_Line1;		/* SP mixer LINE1 setting D4SS1_SP_AMIX		*/
	unsigned char bSpMixer_Line2;		/* SP mixer LINE2 setting D4SS1_SP_BMIX	*/
} D4SS1_SETTING_INFO;
/********************************************************************************************/

/* D-4HP3 Control module API */
/********************************************************************************************/
void D4SS1_PowerOn(D4SS1_SETTING_INFO *pstSettingInfo);		/* power on function */
void D4SS1_PowerOff(void);									/* power off function */
void D4SS1_ControlMixer(unsigned char bHpFlag, unsigned char bSpFlag, D4SS1_SETTING_INFO *pstSetMixer);	/* control mixer function */
void D4SS1_WriteRegisterBit(unsigned long bName, unsigned char bPrm);		/* 1bit write register function */
void D4SS1_WriteRegisterByte(unsigned char bAddress, unsigned char bPrm);	/* 1byte write register function */
void D4SS1_WriteRegisterByteN(unsigned char bAddress, unsigned char *pbPrm, unsigned char bPrmSize);	/* N byte write register function */
void D4SS1_ReadRegisterBit(unsigned long bName, unsigned char *pbPrm);		/* 1bit read register function */
void D4SS1_ReadRegisterByte(unsigned char bAddress, unsigned char *pbPrm);	/* 1byte read register function */
/********************************************************************************************/

struct snd_set_ampgain {
	int in1_gain;
	int in2_gain;
	int hp_att;
	int sp_att;
};

struct yda173_i2c_data {
	struct snd_set_ampgain *ampgain;
	int num_modes;
};

void yda173_speaker_onoff(int onoff); 			/* speaker path amp onoff */
void yda173_speaker_onoff_incall(int onoff); 			/* speaker path amp onoff in a call */
void yda173_speaker_onoff_inVoip(int onoff);
void yda173_headset_onoff(int onoff); 			/* headset path amp onoff */
void yda173_speaker_headset_onoff(int onoff); 	/* speaker+headset path amp onoff */
void yda173_tty_onoff(int onoff); 				/* tty path amp onoff */

#define SND_IOCTL_MAGIC 's'
#define SND_SET_AMPGAIN _IOW(SND_IOCTL_MAGIC, 2, int mode)

#endif
