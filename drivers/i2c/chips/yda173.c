/****************************************************************************
 *
 *		CONFIDENTIAL
 *
 *		Copyright (c) 2011 Yamaha Corporation
 *
 *		Module		: D4SS1_Ctrl.c
 *
 *		Description	: D-4SS1 control module
 *
 *		Version		: 1.0.0 	2011.11.24
 *
 ****************************************************************************/
#include <linux/i2c.h>
#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <mach/vreg.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/i2c/yda173.h>
#include <linux/syscalls.h>


#define MODULE_NAME "yda173"
#define AMPREG_DEBUG 0
#define MODE_NUM_MAX 30

#ifndef TARGET_BUILD_USER
#define AMP_TUNING
#endif

static struct yda173_i2c_data g_data;

extern int charging_boot; //shim

#ifdef AMP_TUNING
static int amptuningFlag = 0;
#define AMP_TUNING_FILE "/data/amp_gain.txt"
#endif 

static struct i2c_client *pclient;
static struct snd_set_ampgain g_ampgain[MODE_NUM_MAX];
static struct snd_set_ampgain temp;
static int set_mode = 0;
static int cur_mode = 0;

static ssize_t mode_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", set_mode);
}
static ssize_t mode_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg < MODE_NUM_MAX ) )
	{
		set_mode = reg;	
	}
	return count;
}

static ssize_t in1_gain_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.in1_gain);
}
static ssize_t in1_gain_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 7 ) )
	{
		temp.in1_gain = reg;
	}
	return count;
}
static ssize_t in2_gain_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.in2_gain);
}
static ssize_t in2_gain_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 7 ) )
	{
		temp.in2_gain = reg;
	}
	return count;
}
static ssize_t hp_att_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.hp_att);
}
static ssize_t hp_att_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 127 ) )
	{
		temp.hp_att = reg;
	}
	return count;
}

static ssize_t sp_att_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d\n", temp.sp_att);
}
static ssize_t sp_att_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if( ( reg >= 0 ) && ( reg <= 127 ) )
	{
		temp.sp_att = reg;
	}
	return count;
}

static ssize_t gain_all_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "%d,%d,%d,%d,%d\n",
											  set_mode,
											  temp.in1_gain ,
                                              temp.in2_gain ,
                                              temp.hp_att   , 
                                              temp.sp_att);
}
static ssize_t save_store(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
    int reg = simple_strtoul(buf, NULL, 10);
	if(reg == 1)
	{
		memcpy(&g_ampgain[set_mode], &temp, sizeof(struct snd_set_ampgain));
	}
		
	return count;
}

static DEVICE_ATTR(mode		, S_IRUGO|S_IWUSR, mode_show, 		mode_store);
static DEVICE_ATTR(in1_gain	, S_IRUGO|S_IWUSR, in1_gain_show, 	in1_gain_store);
static DEVICE_ATTR(in2_gain	, S_IRUGO|S_IWUSR, in2_gain_show, 	in2_gain_store);
static DEVICE_ATTR(hp_att	, S_IRUGO|S_IWUSR, hp_att_show, 	hp_att_store);
static DEVICE_ATTR(sp_att	, S_IRUGO|S_IWUSR, sp_att_show, 	sp_att_store);
static DEVICE_ATTR(gain_all	, S_IRUGO, 				   gain_all_show, 	NULL);
static DEVICE_ATTR(save		, S_IWUSR, 				   NULL, 			save_store);

static struct attribute *yda173_attributes[] = {
    &dev_attr_mode.attr,
    &dev_attr_in1_gain.attr,
    &dev_attr_in2_gain.attr,
    &dev_attr_hp_att.attr,
    &dev_attr_sp_att.attr,
    &dev_attr_gain_all.attr,
    &dev_attr_save.attr,
    NULL
};

static struct attribute_group yda173_attribute_group = {
    .attrs = yda173_attributes
};

static int load_ampgain(void)
{
	struct yda173_i2c_data *yd ;
	int numofmodes;
	int index=0;
	yd = &g_data;
	numofmodes = yd->num_modes;
	for(index=0; index<numofmodes; index++)
	{
		memcpy(&g_ampgain[index], &yd->ampgain[index], sizeof(struct snd_set_ampgain));
#if AMPREG_DEBUG
		pr_info(MODULE_NAME ":[%d].in1_gain = %d \n",index,g_ampgain[index].in1_gain  );
		pr_info(MODULE_NAME ":[%d].in2_gain = %d \n",index,g_ampgain[index].in2_gain  );
		pr_info(MODULE_NAME ":[%d].hp_att   = %d \n",index,g_ampgain[index].hp_att    );
		pr_info(MODULE_NAME ":[%d].sp_att   = %d \n",index,g_ampgain[index].sp_att    );
#endif
	}
	memcpy(&temp, &g_ampgain[0], sizeof(struct snd_set_ampgain));
#if AMPREG_DEBUG
	pr_info(MODULE_NAME ":temp.in1_gain = %d \n",temp.in1_gain  );
	pr_info(MODULE_NAME ":temp.in2_gain = %d \n",temp.in2_gain  );
	pr_info(MODULE_NAME ":temp.hp_att   = %d \n",temp.hp_att    );
	pr_info(MODULE_NAME ":temp.sp_att   = %d \n",temp.sp_att    );
#endif

	pr_info(MODULE_NAME ":%s completed\n",__func__);
	return 0;
}

/*******************************************************************************
 *	d4Write
 *
 *	Function:
 *			write register parameter function
 *	Argument:
 *			UINT8 bWriteRA  : register address
 *			UINT8 bWritePrm : register parameter
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
SINT32 d4Write(UINT8 bWriteRA, UINT8 bWritePrm)
{
	/* Write 1 byte data to the register for each system. */

	int rc = 0;
	
	if(pclient == NULL)
	{
		pr_err(MODULE_NAME ": i2c_client error\n");
		return -1;
	}

	rc = i2c_smbus_write_byte_data(pclient, bWriteRA, bWritePrm); 
	if(rc)
	{
		pr_err(MODULE_NAME ": i2c write error %d\n", rc);
		return rc;
	}

	return 0;
}

/*******************************************************************************
 *	d4WriteN
 *
 *	Function:
 *			write register parameter function
 *	Argument:
 *			UINT8 bWriteRA    : register address
 *			UINT8 *pbWritePrm : register parameter
 *			UINT8 bWriteSize  : size of "*pbWritePrm"
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
SINT32 d4WriteN(UINT8 bWriteRA, UINT8 *pbWritePrm, UINT8 bWriteSize)
{
	/* Write N byte data to the register for each system. */
	
	int rc = 0;
	
	if(pclient == NULL)
	{
		pr_err(MODULE_NAME ": i2c_client error\n");
		return -1;
	}

	rc = i2c_smbus_write_block_data(pclient, bWriteRA, bWriteSize, pbWritePrm); 
	if(rc)
	{
		pr_err(MODULE_NAME ": i2c write error %d\n", rc);
		return rc;
	}

	return 0;
	
}

/*******************************************************************************
 *	d4Read
 *
 *	Function:
 *			read register parameter function
 *	Argument:
 *			UINT8 bReadRA    : register address
 *			UINT8 *pbReadPrm : register parameter
 *
 *	Return:
 *			SINT32	>= 0 success
 *					<  0 error
 *
 ******************************************************************************/
SINT32 d4Read(UINT8 bReadRA, UINT8 *pbReadPrm)
{
	/* Read byte data to the register for each system. */
	
	int buf;
	
	if(pclient == NULL)
	{
		pr_err(MODULE_NAME ": i2c_client error\n");
		return -1;
	}

	buf  = i2c_smbus_read_byte_data(pclient, bReadRA);	
	if(buf < 0)
	{
		pr_err(MODULE_NAME ": i2c read error %d\n", buf);
		return buf;
	}
	
	*pbReadPrm = (UINT8)buf;

	return 0;
}

/*******************************************************************************
 *	d4Wait
 *
 *	Function:
 *			wait function
 *	Argument:
 *			UINT32 dTime : wait time [ micro second ]
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void d4Wait(UINT32 dTime)
{
	/* Wait procedure for each system */
	
	udelay(dTime);
}

/*******************************************************************************
 *	d4Sleep
 *
 *	Function:
 *			sleep function
 *	Argument:
 *			UINT32 dTime : sleep time [ milli second ]
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void d4Sleep(UINT32 dTime)
{
	/* Sleep procedure for each system */

	msleep(dTime);
}

/*******************************************************************************
 *	d4ErrHandler
 *
 *	Function:
 *			error handler function
 *	Argument:
 *			SINT32 dError : error code
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void d4ErrHandler(SINT32 dError)
{
	/* Error procedure for each system */

	pr_err(MODULE_NAME ": %s error %ld\n", __func__, dError);
}

/* D-4HP3 register map */
UINT8 g_bD4SS1RegisterMap[10] = 
{	
	0x84,	/* 0x80 : SRST(7)=1, CT_CAN(6-5)=0, PPDOFF(4)=0, MODESEL(3)=0, CPMODE(2)=1, VLEVEL(0)=0 */
	0x1C,	/* 0x81 : DRC_MODE(7-6)=0, DATRT(5-4)=1, NG_ATRT(3-2)=3 */
	0x10,	/* 0x82 : DPLT(7-5)=0, HP_NG_RAT(4-2)=4, HP_NG_ATT(1-0)=0 */
	0x10,	/* 0x83 : DPLT_EX(6)=0, NCLIP(5)=0, SP_NG_RAT(4-2)=4, SP_NG_ATT(1-0)=0 */
	0x22,	/* 0x84 : VA(7-4)=2, VB(3-0)=2 */
	0x04,	/* 0x85 : DIFA(7)=0, DIFB(6)=0, HP_SVOFF(3)=0, HP_HIZ(2)=1, SP_SVOFF(1)=0, SP_HIZ(0)=0 */
	0x00,	/* 0x86 : VOLMODE(7)=0, SP_ATT(6-0)=0 */
	0x00,	/* 0x87 : HP_ATT(6-0)=0 */
	0x00	/* 0x88 : OCP_ERR(7)=0, OTP_ERR(6)=0, DC_ERR(5)=0, HP_MONO(4)=0, HP_AMIX(3)=0, HP_BMIX(2)=0, SP_AMIX(1)=0, SP_BMIX(0)=0 */
};

/*******************************************************************************
 *	D4SS1_UpdateRegisterMap
 *
 *	Function:
 *			update register map (g_bD4SS1RegisterMap[]) function
 *	Argument:
 *			SINT32	sdRetVal	: update flag
 *			UINT8	bRN			: register number (0 - 8)
 *			UINT8	bPrm		: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4SS1_UpdateRegisterMap( SINT32 sdRetVal, UINT8 bRN, UINT8 bPrm )
{
	if(sdRetVal < 0)
	{
		d4ErrHandler( D4SS1_ERROR );
	}
	else
	{
		/* update register map */
		g_bD4SS1RegisterMap[ bRN ] = bPrm;
	}
}

/*******************************************************************************
 *	D4SS1_UpdateRegisterMapN
 *
 *	Function:
 *			update register map (g_bD4SS1RegisterMap[]) function
 *	Argument:
 *			SINT32	sdRetVal	: update flag
 *			UINT8	bRN			: register number(0 - 8)
 *			UINT8	*pbPrm		: register parameter
 *			UINT8	bPrmSize	: size of " *pbPrm"
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
static void D4SS1_UpdateRegisterMapN( SINT32 sdRetVal, UINT8 bRN, UINT8 *pbPrm, UINT8 bPrmSize )
{
	UINT8 bCnt = 0;

	if(sdRetVal < 0)
	{
		d4ErrHandler( D4SS1_ERROR );
	}
	else
	{
		/* update register map */
		for(bCnt = 0; bCnt < bPrmSize; bCnt++)
		{
			g_bD4SS1RegisterMap[ bRN + bCnt ] = pbPrm[ bCnt ];
		}
	}
}

/*******************************************************************************
 *	D4SS1_WriteRegisterBit
 *
 *	Function:
 *			write register "bit" function
 *	Argument:
 *			UINT32	dName	: register name
 *			UINT8	bPrm	: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_WriteRegisterBit( UINT32 dName, UINT8 bPrm )
{
	UINT8 bWritePrm;			/* I2C sending parameter */
	UINT8 bDummy;				/* setting parameter */
	UINT8 bRA, bRN, bMB, bSB;	/* register address, register number, mask bit, shift bit */

	/* 
	dName
	bit 31 - 16 : register address
	bit 15 -  8	: mask bit
	bit  7 -  0	: shift bit
	*/
	bRA = (UINT8)(( dName & 0xFF0000 ) >> 16);
	bRN = bRA - 0x80;
	bMB = (UINT8)(( dName & 0x00FF00 ) >> 8);
	bSB = (UINT8)( dName & 0x0000FF );

	/* check arguments */
	if((bRA < D4SS1_MIN_REGISTERADDRESS) && (D4SS1_MAX_WRITE_REGISTERADDRESS < bRA))
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
	/* set register parameter */
	bPrm = (bPrm << bSB) & bMB;
	bDummy = bMB ^ 0xFF;
	bWritePrm = g_bD4SS1RegisterMap[ bRN ] & bDummy;	/* set bit of writing position to 0 */
	bWritePrm = bWritePrm | bPrm;						/* set parameter of writing bit */
	/* call the user implementation function "d4Write()", and write register */
	D4SS1_UpdateRegisterMap( d4Write( bRA, bWritePrm ), bRN, bWritePrm );
}

/*******************************************************************************
 *	D4SS1_WriteRegisterByte
 *
 *	Function:
 *			write register "byte" function
 *	Argument:
 *			UINT8 bAddress  : register address
 *			UINT8 bPrm : register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_WriteRegisterByte(UINT8 bAddress, UINT8 bPrm)
{
	UINT8 bNumber;
	/* check arguments */
	if((bAddress < D4SS1_MIN_REGISTERADDRESS) && (D4SS1_MAX_WRITE_REGISTERADDRESS < bAddress))
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
	bNumber = bAddress - 0x80;
	D4SS1_UpdateRegisterMap( d4Write( bAddress, bPrm ), bNumber, bPrm );
}

/*******************************************************************************
 *	D4SS1_WriteRegisterByteN
 *
 *	Function:
 *			write register "n byte" function
 *	Argument:
 *			UINT8 bAddress	: register address
 *			UINT8 *pbPrm	: register parameter
 *			UINT8 bPrmSize	: size of "*pbPrm"
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_WriteRegisterByteN(UINT8 bAddress, UINT8 *pbPrm, UINT8 bPrmSize)
{
	UINT8 bNumber;
	/* check arguments */
	if((bAddress < D4SS1_MIN_REGISTERADDRESS) && (D4SS1_MAX_WRITE_REGISTERADDRESS < bAddress))
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
	if( bPrmSize > ((D4SS1_MAX_WRITE_REGISTERADDRESS - D4SS1_MIN_REGISTERADDRESS) + 1))
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
	bNumber = bAddress - 0x80;
	D4SS1_UpdateRegisterMapN( d4WriteN( bAddress, pbPrm, bPrmSize ), bNumber, pbPrm, bPrmSize);
}

/*******************************************************************************
 *	D4SS1_ReadRegisterBit
 *
 *	Function:
 *			read register "bit" function
 *	Argument:
 *			UINT32 dName	: register name
 *			UINT8  *pbPrm	: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_ReadRegisterBit( UINT32 dName, UINT8 *pbPrm)
{
	SINT32 sdRetVal = D4SS1_SUCCESS;
	UINT8 bRA, bRN, bMB, bSB;	/* register address, register number, mask bit, shift bit */

	/* 
	dName
	bit 31 - 16	: register address
	bit 15 -  8	: mask bit
	bit  7 -  0	: shift bit
	*/
	bRA = (UINT8)(( dName & 0xFF0000 ) >> 16);
	bRN = bRA - 0x80;
	bMB = (UINT8)(( dName & 0x00FF00 ) >> 8);
	bSB = (UINT8)( dName & 0x0000FF );

	/* check arguments */
	if((bRA < D4SS1_MIN_REGISTERADDRESS) && (D4SS1_MAX_READ_REGISTERADDRESS < bRA))
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
	/* call the user implementation function "d4Read()", and read register */
	sdRetVal = d4Read( bRA, pbPrm );
	D4SS1_UpdateRegisterMap( sdRetVal, bRN, *pbPrm );
	/* extract the parameter of selected register in the read register parameter */
	*pbPrm = ((g_bD4SS1RegisterMap[ bRN ] & bMB) >> bSB);
}

/*******************************************************************************
 *	D4SS1_ReadRegisterByte
 *
 *	Function:
 *			read register "byte" function
 *	Argument:
 *			UINT8 bAddress	: register address
 *			UINT8 *pbPrm	: register parameter
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_ReadRegisterByte( UINT8 bAddress, UINT8 *pbPrm)
{
	SINT32 sdRetVal = D4SS1_SUCCESS;
	UINT8 bNumber;
	/* check arguments */
	if((bAddress < D4SS1_MIN_REGISTERADDRESS) && (D4SS1_MAX_READ_REGISTERADDRESS < bAddress))
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
	/* call the user implementation function "d4Read()", and read register */
	bNumber = bAddress - 0x80;
	sdRetVal = d4Read( bAddress, pbPrm );
	D4SS1_UpdateRegisterMap( sdRetVal, bNumber, *pbPrm );
}


/*******************************************************************************
 *	D4SS1_CheckArgument_Mixer
 *
 *	Function:
 *			check D-4HP3 setting information for mixer
 *	Argument:
 *			UINT8 bHpFlag : "change HP amp mixer setting" flag(0 : no check, 1 : check)
 *			UINT8 bSpFlag : "change SP amp mixer setting" flag(0 : no check, 1 : check)
 *			D4SS1_SETTING_INFO *pstSettingInfo : D-4HP3 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_CheckArgument_Mixer(UINT8 bHpFlag, UINT8 bSpFlag, D4SS1_SETTING_INFO *pstSettingInfo)
{
	UINT8 bCheckArgument = 0;

	/* HP */
	if(bHpFlag == 1)
	{
		if(pstSettingInfo->bHpCh > 1)
		{
			pstSettingInfo->bHpCh = 0;
			bCheckArgument++;
		}
		if(pstSettingInfo->bHpMixer_Line1 > 1)
		{
			pstSettingInfo->bHpMixer_Line1 = 0;
			bCheckArgument++;
		}
		if(pstSettingInfo->bHpMixer_Line2 > 1)
		{
			pstSettingInfo->bHpMixer_Line2 = 0;
			bCheckArgument++;
		}
	}

	/* SP */
	if(bSpFlag == 1)
	{
		if(pstSettingInfo->bSpMixer_Line1 > 1)
		{
			pstSettingInfo->bSpMixer_Line1 = 0;
			bCheckArgument++;
		}
		if(pstSettingInfo->bSpMixer_Line2 > 1)
		{
			pstSettingInfo->bSpMixer_Line2 = 0;
			bCheckArgument++;
		}
	}
	
	/* check argument */
	if(bCheckArgument > 0)
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
}

/*******************************************************************************
 *	D4SS1_CheckArgument
 *
 *	Function:
 *			check D-4HP3 setting information
 *	Argument:
 *			D4SS1_SETTING_INFO *pstSettingInfo : D-4HP3 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_CheckArgument(D4SS1_SETTING_INFO *pstSettingInfo)
{
	UINT8 bCheckArgument = 0;

	/* IN */
	if(pstSettingInfo->bLine1Gain > 12)
	{
		pstSettingInfo->bLine1Gain = 2;
		bCheckArgument++;
	}
	if(pstSettingInfo->bLine2Gain > 12)
	{
		pstSettingInfo->bLine2Gain = 2;
		bCheckArgument++;
	}
	if(pstSettingInfo->bLine1Balance > 1)
	{
		pstSettingInfo->bLine1Balance = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bLine2Balance > 1)	
	{
		pstSettingInfo->bLine2Balance = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bLineVolMode > 1)	
	{
		pstSettingInfo->bLineVolMode = 0;
		bCheckArgument++;
	}
	/* HP */
	if(pstSettingInfo->bHpCtCancel > 3)
	{
		pstSettingInfo->bHpCtCancel = 0;
		bCheckArgument++;
	}	
	if(pstSettingInfo->bHpPpdOff > 1)
	{
		pstSettingInfo->bHpPpdOff = 0;
		bCheckArgument++;
	}	
	if(pstSettingInfo->bHpModeSel > 1)
	{
		pstSettingInfo->bHpModeSel = 0;
		bCheckArgument++;
	}	
	if(pstSettingInfo->bHpCpMode > 1)
	{
		pstSettingInfo->bHpCpMode = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpAvddLev > 1)
	{
		pstSettingInfo->bHpAvddLev = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpNg_AttackTime > 3)
	{
		pstSettingInfo->bHpNg_AttackTime = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpNg_Rat > 7)
	{
		pstSettingInfo->bHpNg_Rat = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpNg_Att > 3)
	{
		pstSettingInfo->bHpNg_Att = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpSvol > 1)
	{
		pstSettingInfo->bHpSvol = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpHiZ > 1)
	{
		pstSettingInfo->bHpHiZ = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpAtt > 127)
	{
		pstSettingInfo->bHpAtt = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpCh > 1)
	{
		pstSettingInfo->bHpCh = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpMixer_Line1 > 1)
	{
		pstSettingInfo->bHpMixer_Line1 = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bHpMixer_Line2 > 1)
	{
		pstSettingInfo->bHpMixer_Line2 = 0;
		bCheckArgument++;
	}

	/* SP */
	if(pstSettingInfo->bSpModeSel > 1)
	{
		pstSettingInfo->bSpModeSel = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpDrcMode > 3)
	{
		pstSettingInfo->bSpDrcMode = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNcpl_AttackTime > 3)
	{
		pstSettingInfo->bSpNcpl_AttackTime = 1;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNcpl_ReleaseTime > 1)
	{
		pstSettingInfo->bSpNcpl_ReleaseTime = 1;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNg_AttackTime > 3)
	{
		pstSettingInfo->bSpNg_AttackTime = 3;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNcpl_PowerLimit > 10)
	{
		pstSettingInfo->bSpNcpl_PowerLimit =0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNcpl_PowerLimitEx > 10)
	{
		pstSettingInfo->bSpNcpl_PowerLimitEx =0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNclip > 1)
	{
		pstSettingInfo->bSpNclip = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNg_Rat > 7)
	{
		pstSettingInfo->bSpNg_Rat = 4;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpNg_Att > 3)
	{
		pstSettingInfo->bSpNg_Att = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpSvol > 1)
	{
		pstSettingInfo->bSpSvol = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpHiZ > 1)
	{
		pstSettingInfo->bSpHiZ = 0;
		bCheckArgument++;
	}

	if(pstSettingInfo->bSpAtt > 127)
	{
		pstSettingInfo->bSpAtt = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpMixer_Line1 > 1)
	{
		pstSettingInfo->bSpMixer_Line1 = 0;
		bCheckArgument++;
	}
	if(pstSettingInfo->bSpMixer_Line2 > 1)
	{
		pstSettingInfo->bSpMixer_Line2 = 0;
		bCheckArgument++;
	}

	/* check argument */
	if(bCheckArgument > 0)
	{
		d4ErrHandler( D4SS1_ERROR_ARGUMENT );
	}
}


/*******************************************************************************
 *	D4SS1_ControlMixer
 *
 *	Function:
 *			control HP amp mixer and SP amp mixer in D-4HP3
 *	Argument:
 *			UINT8 bHpFlag : "change HP amp mixer setting" flag(0 : no change, 1 : change)
 *			UINT8 bSpFlag : "change SP amp mixer setting" flag(0 : no change, 1 : change)
 *			D4SS1_SETTING_INFO *pstSetMixer : D-4HP3 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_ControlMixer(UINT8 bHpFlag, UINT8 bSpFlag, D4SS1_SETTING_INFO *pstSetMixer)
{
	UINT8 bWriteRA, bWritePrm;
	UINT8 bTempHpCh, bTempHpMixer_Line1, bTempHpMixer_Line2;
	UINT8 bTempSpMixer_Line1, bTempSpMixer_Line2;

	/* check argument */
	if( bHpFlag > 1 )
	{
		bHpFlag = 0;
	}
	if( bSpFlag > 1 )
	{
		bSpFlag = 0;
	}
	D4SS1_CheckArgument_Mixer( bHpFlag, bSpFlag, pstSetMixer );

	/* change mixer sequence */
	/* SP */
	if(bSpFlag == 1)
	{
		bTempSpMixer_Line1 = 0;
		bTempSpMixer_Line2 = 0;
	}
	else
	{
		bTempSpMixer_Line1 = (g_bD4SS1RegisterMap[8] & 0x02) >> (D4SS1_SP_AMIX & 0xFF);
		bTempSpMixer_Line2 = (g_bD4SS1RegisterMap[8] & 0x01) >> (D4SS1_SP_BMIX & 0xFF);
	}

	/* HP */
	bTempHpCh = (g_bD4SS1RegisterMap[8] & 0x10) >> (D4SS1_HP_MONO & 0xFF);
	if(bHpFlag == 1)
	{
		bTempHpMixer_Line1 = 0;
		bTempHpMixer_Line2 = 0;
	}
	else
	{
		bTempHpMixer_Line1 = (g_bD4SS1RegisterMap[8] & 0x08) >> (D4SS1_HP_AMIX & 0xFF);
		bTempHpMixer_Line2 = (g_bD4SS1RegisterMap[8] & 0x04) >> (D4SS1_HP_BMIX & 0xFF);
	}

	/* write register #0x88 */
	bWriteRA = 0x88;
	bWritePrm = (bTempHpCh << 4) 											/* HP_MONO */
				| (bTempHpMixer_Line1 << 3) | (bTempHpMixer_Line2 << 2) /* HP_AMIX, HP_BMIX */
				| (bTempSpMixer_Line1 << 1) | (bTempSpMixer_Line2);		/* SP_AMIX, SP_BMIX */

	D4SS1_WriteRegisterByte(bWriteRA, bWritePrm);

	/* set HP amp mixer, SP amp mixer */
	if(bHpFlag == 1)
	{
		bTempHpCh = pstSetMixer->bHpCh;
		bTempHpMixer_Line1 = pstSetMixer->bHpMixer_Line1;
		bTempHpMixer_Line2 = pstSetMixer->bHpMixer_Line2;
	}
	if(bSpFlag == 1)
	{
		bTempSpMixer_Line1 = pstSetMixer->bSpMixer_Line1;
		bTempSpMixer_Line2 = pstSetMixer->bSpMixer_Line2;
	}

	/* write register #0x88 */
	if((bHpFlag == 1) || (bSpFlag == 1))
	{
		bWritePrm = (bTempHpCh << 4)						 											/* HP_MONO */
					| (bTempHpMixer_Line1 << 3) | (bTempHpMixer_Line2 << 2) /* HP_AMIX, HP_BMIX */
					| (bTempSpMixer_Line1 << 1) | (bTempSpMixer_Line2);			/* SP_AMIX, SP_BMIX */
		bWriteRA = 0x88;
		D4SS1_WriteRegisterByte(bWriteRA, bWritePrm);
	}
}

/*******************************************************************************
 *	D4SS1_PowerOn
 *
 *	Function:
 *			power on D-4HP3
 *	Argument:
 *			D4SS1_SETTING_INFO *pstSettingInfo : D-4HP3 setting information
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_PowerOn(D4SS1_SETTING_INFO *pstSettingInfo)
{
	//UINT8 bWriteAddress;
	UINT8 abWritePrm[9];
	/* check argument */
	D4SS1_CheckArgument( pstSettingInfo );

	/* set parameter */
	//bWriteAddress = 0x80;
	/* 0x80 */
	abWritePrm[0] = (pstSettingInfo->bHpCtCancel << (D4SS1_CT_CAN & 0xFF))
					| (pstSettingInfo->bHpPpdOff << (D4SS1_PPDOFF & 0xFF))
					| (pstSettingInfo->bHpModeSel << (D4SS1_MODESEL & 0xFF))
					| (pstSettingInfo->bSpModeSel << (D4SS1_MODESEL & 0xFF))
					| (pstSettingInfo->bHpCpMode << (D4SS1_CPMODE & 0xFF))
					| (pstSettingInfo->bHpAvddLev << (D4SS1_VLEVEL & 0xFF));
	/* 0x81 */
	abWritePrm[1] = (pstSettingInfo->bSpDrcMode << (D4SS1_DRC_MODE & 0xFF))
					| (pstSettingInfo->bSpNcpl_AttackTime << (D4SS1_DATRT & 0xFF))
					| (pstSettingInfo->bHpNg_AttackTime << (D4SS1_NG_ATRT & 0xFF))
					| (pstSettingInfo->bSpNg_AttackTime << (D4SS1_NG_ATRT & 0xFF));

	/* 0x82 */
	abWritePrm[2] = (pstSettingInfo->bSpNcpl_PowerLimit << (D4SS1_DPLT & 0xFF))
					| (pstSettingInfo->bHpNg_Rat << (D4SS1_HP_NG_RAT & 0xFF)) 
					| (pstSettingInfo->bHpNg_Att << (D4SS1_HP_NG_ATT & 0xFF));
	/* 0x83 */
	abWritePrm[3] = (pstSettingInfo->bSpNcpl_PowerLimitEx << (D4SS1_DPLT_EX & 0xFF))
					| (pstSettingInfo->bSpNclip << (D4SS1_NCLIP & 0xFF))
					| (pstSettingInfo->bSpNg_Rat << (D4SS1_SP_NG_RAT & 0xFF)) 
					| (pstSettingInfo->bSpNg_Att << (D4SS1_SP_NG_ATT & 0xFF));
	/* 0x84 */
	abWritePrm[4] = (pstSettingInfo->bLine1Gain << (D4SS1_VA & 0xFF))
					| (pstSettingInfo->bLine2Gain << (D4SS1_VB & 0xFF));
    /* 0x85 */
	abWritePrm[5] = (pstSettingInfo->bLine1Balance << (D4SS1_DIFA & 0xFF))
					| (pstSettingInfo->bLine2Balance << (D4SS1_DIFB & 0xFF))
					| (pstSettingInfo->bHpSvol << (D4SS1_HP_SVOFF & 0xFF))
					| (pstSettingInfo->bHpHiZ << (D4SS1_HP_HIZ & 0xFF))
					| (pstSettingInfo->bSpSvol << (D4SS1_SP_SVOFF & 0xFF))
					| (pstSettingInfo->bSpHiZ << (D4SS1_SP_HIZ & 0xFF));
    /* 0x86 */
	abWritePrm[6] = (pstSettingInfo->bLineVolMode << (D4SS1_VOLMODE & 0xFF))
					| (pstSettingInfo->bSpAtt << (D4SS1_SP_ATT & 0xFF));
	/* 0x87 */
	abWritePrm[7] = (pstSettingInfo->bHpAtt << (D4SS1_HP_ATT & 0xFF));
	/* 0x88 */
	abWritePrm[8] = (pstSettingInfo->bHpCh << (D4SS1_HP_MONO & 0xFF))
					| (pstSettingInfo->bHpMixer_Line1 << (D4SS1_HP_AMIX & 0xFF))
					| (pstSettingInfo->bHpMixer_Line2 << (D4SS1_HP_BMIX & 0xFF))
					| (pstSettingInfo->bSpMixer_Line1 << (D4SS1_SP_AMIX & 0xFF))
					| (pstSettingInfo->bSpMixer_Line2 << (D4SS1_SP_BMIX & 0xFF));
#if AMPREG_DEBUG
	pr_info(MODULE_NAME ": 0x80 = %02x\n", abWritePrm[0]);
	pr_info(MODULE_NAME ": 0x81 = %02x\n", abWritePrm[1]);
	pr_info(MODULE_NAME ": 0x82 = %02x\n", abWritePrm[2]);
	pr_info(MODULE_NAME ": 0x83 = %02x\n", abWritePrm[3]);
	pr_info(MODULE_NAME ": 0x84 = %02x\n", abWritePrm[4]);
	pr_info(MODULE_NAME ": 0x85 = %02x\n", abWritePrm[5]);
	pr_info(MODULE_NAME ": 0x86 = %02x\n", abWritePrm[6]);
	pr_info(MODULE_NAME ": 0x87 = %02x\n", abWritePrm[7]);
	pr_info(MODULE_NAME ": 0x88 = %02x\n", abWritePrm[8]);
#endif

	/* write 0x80 - 0x88 : power on */
#if 1
	D4SS1_WriteRegisterByte(0x80, abWritePrm[0]);
	D4SS1_WriteRegisterByte(0x81, abWritePrm[1]);
	D4SS1_WriteRegisterByte(0x82, abWritePrm[2]);
	D4SS1_WriteRegisterByte(0x83, abWritePrm[3]);
	D4SS1_WriteRegisterByte(0x84, abWritePrm[4]);
	D4SS1_WriteRegisterByte(0x85, abWritePrm[5]);
	D4SS1_WriteRegisterByte(0x86, abWritePrm[6]);
	D4SS1_WriteRegisterByte(0x87, abWritePrm[7]);
	D4SS1_WriteRegisterByte(0x88, abWritePrm[8]);
#else
	D4SS1_WriteRegisterByteN(bWriteAddress, abWritePrm, 8);
#endif
}

/*******************************************************************************
 *	D4SS1_PowerOff
 *
 *	Function:
 *			power off D-4HP3
 *	Argument:
 *			none
 *
 *	Return:
 *			none
 *
 ******************************************************************************/
void D4SS1_PowerOff(void)
{
	UINT8 bWriteAddress;
	UINT8 bWritePrm;

	/* 0x87 : power off HP amp, SP amp */
	bWriteAddress = 0x88;
	bWritePrm = 0x00;
	D4SS1_WriteRegisterByte(bWriteAddress, bWritePrm);

	d4Sleep(D4SS1_OFFSEQUENCE_WAITTIME);
}

#ifdef AMP_TUNING
void Amp_gain_tuning(void)
{

	char buff[48];
	int fd;
    mm_segment_t segment = get_fs();

	
	amptuningFlag = 1;
    set_fs(KERNEL_DS);
	
	fd = sys_open(AMP_TUNING_FILE, O_RDONLY, 0);
	
    if(fd >= 0)
    {
		memset(buff,0x0,sizeof(struct snd_set_ampgain)*2);
		sys_read(fd,buff,sizeof(struct snd_set_ampgain)*2);
		memcpy(&g_ampgain, buff, sizeof(struct snd_set_ampgain)*2);
 
        sys_close(fd);
    }
	else
	{
		fd = sys_open(AMP_TUNING_FILE, O_WRONLY | O_CREAT, 0777);
		
		if(fd >= 0)
		{
			memset(buff,0x0,sizeof(struct snd_set_ampgain)*2);
			memcpy(buff, &g_ampgain, sizeof(struct snd_set_ampgain)*2);
			sys_write(fd, buff, sizeof(struct snd_set_ampgain)*2);
			sys_close(fd);
		}
	}
    set_fs(segment);

}
#endif

void yda173_speaker_onoff(int onoff) /* speaker path amp onoff */
{
	D4SS1_SETTING_INFO stInfo;
#if AMPREG_DEBUG	
	unsigned char buf;
#endif
	
	if (onoff)
	{
		#ifdef AMP_TUNING
		if(!amptuningFlag)
		{
			Amp_gain_tuning();
		}
		#endif
		
		pr_info(MODULE_NAME ":speaker on\n");
	/* input */
	stInfo.bLine1Gain=g_ampgain[cur_mode].in1_gain;		/* LINE1 Gain Amp D4SS1_VA	*/	
	stInfo.bLine2Gain=g_ampgain[cur_mode].in2_gain;		/* LINE2 Gain Amp D4SS1_VB	*/	

	stInfo.bLine1Balance=0;	/* LINE1 Single-ended(0) or Differential(1) D4SS1_DIFA*/
	stInfo.bLine2Balance=0;	/* LINE2 Single-ended(0) or Differential(1) D4SS1_DIFB*/

	stInfo.bLineVolMode=0;		/* How input are use  D4SS1_VOLMODE		*/
	/*HP*/
	stInfo.bHpCtCancel=0;			/* HP CrossTalk Cancel D4SS1_CT_CAN*/
	stInfo.bHpPpdOff=0;			/* HP partial powerdownfunction D4SS1_PPDOFF*/
	stInfo.bHpModeSel=0;			/* HP enable sound output within 10ms D4SS1_MODESEL*/		
	stInfo.bHpCpMode=0;			/* HP charge pump mode setting, 3stage mode(0) / 2stage mode(1) D4SS1_CPMODE*/		
	stInfo.bHpAvddLev=0;			/* HP charge pump AVDD level, 1.65V<=AVDD<2.40V(0) / 2.40V<=AVDD<=2.86V(1) D4SS1_VLEVEL*/		

	stInfo.bHpNg_AttackTime=0;		/* HP Noise Gate : attack time D4SS1_NG_ATRT*/

	stInfo.bHpNg_Rat=0; 			/* HP Detection Level of headphone Amplifier's Noise GateD4SS1_HP_NG_RAT	*/	
	stInfo.bHpNg_Att=0; 			/* HP Attenuation Level for headphone Amplifier's Noise GateD4SS1_HP_NG_ATT	*/	

	stInfo.bHpSvol=0;				/* HP soft volume setting, on(0) / off(1) D4SS1_HP_SVOFF		*/
	stInfo.bHpHiZ=1;				/* HP Hiz state D4SS1_HP_HIZ*/

	stInfo.bHpAtt=g_ampgain[cur_mode].hp_att;				/* HP attenuator D4SS1_HP_ATT		*/
			
	stInfo.bHpCh=0;				/* HP channel, stereo(0)/mono(1) D4SS1_HP_MONO		*/
	stInfo.bHpMixer_Line1=0;		/* HP mixer LINE1 setting D4SS1_HP_AMIX		*/
	stInfo.bHpMixer_Line2=0;		/* HP mixer LINE2 setting D4SS1_HP_BMIX		*/

	/*SP*/
	stInfo.bSpModeSel=0;			/* SP enable sound output within 10ms D4SS1_MODESEL		*/
	
	stInfo.bSpDrcMode=0;			/* SP DRC or Non-Clip S function D4SS1_DRC_MODE		*/
	stInfo.bSpNcpl_AttackTime=0;	/* SP Non-Clip power limiter : attack Time D4SS1_DATRT		*/
	stInfo.bSpNcpl_ReleaseTime=0;	/* SP Non-Clip power limiter : release Time D4SS1_DATRT		*/
	stInfo.bSpNg_AttackTime=0;		/* SP Noise Gate : attack time D4SS1_NG_ATRT		*/

	stInfo.bSpNcpl_PowerLimit=0;	/* SP Non-Clip power limiter : Power Limit D4SS1_DPLT		*/

	stInfo.bSpNcpl_PowerLimitEx=0;	/* SP Non-Clip power limiter : Power Limit ExD4SS1_DPLT_EX		*/
	stInfo.bSpNclip=0;				/* SP On/Off Non-Clip funtion D4SS1_NCLIP		*/
	stInfo.bSpNg_Rat=0; 			/* SP Detection Level of headphone Amplifier's Noise GateD4SS1_SP_NG_RAT	*/	
	stInfo.bSpNg_Att=0; 			/* SP Attenuation Level for headphone Amplifier's Noise GateD4SS1_SP_NG_ATT	*/	

	stInfo.bSpSvol=0;				/* SP soft volume setting, on(0) / off(1) D4SS1_SP_SVOFF		*/
	stInfo.bSpHiZ=0;				/* SP HiZ state D4SS1_SP_HIZ	*/

	stInfo.bSpAtt=g_ampgain[cur_mode].sp_att;				/* SP attenuator D4SS1_SP_ATT	*/

	stInfo.bSpMixer_Line1=1;		/* SP mixer LINE1 setting D4SS1_SP_AMIX		*/
	stInfo.bSpMixer_Line2=0;		/* SP mixer LINE2 setting D4SS1_SP_BMIX	*/

		D4SS1_PowerOn(&stInfo);

#if AMPREG_DEBUG
		D4SS1_ReadRegisterByte( 0x80, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x81, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x82, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x83, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x84, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x85, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x86, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x87, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x88, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
#endif
	}
	else
	{
		pr_info(MODULE_NAME ":speaker off\n");
		D4SS1_PowerOff();
	}
}

void yda173_speaker_onoff_incall(int onoff) /* speaker path amp onoff */
{
	D4SS1_SETTING_INFO stInfo;
#if AMPREG_DEBUG	
	unsigned char buf;
#endif
	
	if (onoff)
	{		
		pr_info(MODULE_NAME ":speaker on\n");
		
		/* input */
		stInfo.bLine1Gain=g_ampgain[1].in1_gain; 	/* LINE1 Gain Amp D4SS1_VA	*/	
		stInfo.bLine2Gain=g_ampgain[1].in2_gain; 	/* LINE2 Gain Amp D4SS1_VB	*/	
		
		stInfo.bLine1Balance=0; /* LINE1 Single-ended(0) or Differential(1) D4SS1_DIFA*/
		stInfo.bLine2Balance=0; /* LINE2 Single-ended(0) or Differential(1) D4SS1_DIFB*/
		
		stInfo.bLineVolMode=0;		/* How input are use  D4SS1_VOLMODE 	*/
		/*HP*/
		stInfo.bHpCtCancel=0;			/* HP CrossTalk Cancel D4SS1_CT_CAN*/
		stInfo.bHpPpdOff=0; 		/* HP partial powerdownfunction D4SS1_PPDOFF*/
		stInfo.bHpModeSel=0;			/* HP enable sound output within 10ms D4SS1_MODESEL*/		
		stInfo.bHpCpMode=0; 		/* HP charge pump mode setting, 3stage mode(0) / 2stage mode(1) D4SS1_CPMODE*/		
		stInfo.bHpAvddLev=0;			/* HP charge pump AVDD level, 1.65V<=AVDD<2.40V(0) / 2.40V<=AVDD<=2.86V(1) D4SS1_VLEVEL*/		
		
		stInfo.bHpNg_AttackTime=0;		/* HP Noise Gate : attack time D4SS1_NG_ATRT*/
		
		stInfo.bHpNg_Rat=0; 			/* HP Detection Level of headphone Amplifier's Noise GateD4SS1_HP_NG_RAT	*/	
		stInfo.bHpNg_Att=0; 			/* HP Attenuation Level for headphone Amplifier's Noise GateD4SS1_HP_NG_ATT */	
		
		stInfo.bHpSvol=0;				/* HP soft volume setting, on(0) / off(1) D4SS1_HP_SVOFF		*/
		stInfo.bHpHiZ=1;				/* HP Hiz state D4SS1_HP_HIZ*/
		
		stInfo.bHpAtt=g_ampgain[1].hp_att;				/* HP attenuator D4SS1_HP_ATT		*/
				
		stInfo.bHpCh=0; 			/* HP channel, stereo(0)/mono(1) D4SS1_HP_MONO		*/
		stInfo.bHpMixer_Line1=0;		/* HP mixer LINE1 setting D4SS1_HP_AMIX 	*/
		stInfo.bHpMixer_Line2=0;		/* HP mixer LINE2 setting D4SS1_HP_BMIX 	*/
		
		/*SP*/
		stInfo.bSpModeSel=0;			/* SP enable sound output within 10ms D4SS1_MODESEL 	*/
		
		stInfo.bSpDrcMode=0;			/* SP DRC or Non-Clip S function D4SS1_DRC_MODE 	*/
		stInfo.bSpNcpl_AttackTime=0;	/* SP Non-Clip power limiter : attack Time D4SS1_DATRT		*/
		stInfo.bSpNcpl_ReleaseTime=0;	/* SP Non-Clip power limiter : release Time D4SS1_DATRT 	*/
		stInfo.bSpNg_AttackTime=0;		/* SP Noise Gate : attack time D4SS1_NG_ATRT		*/
		
		stInfo.bSpNcpl_PowerLimit=0;	/* SP Non-Clip power limiter : Power Limit D4SS1_DPLT		*/
		
		stInfo.bSpNcpl_PowerLimitEx=0;	/* SP Non-Clip power limiter : Power Limit ExD4SS1_DPLT_EX		*/
		stInfo.bSpNclip=0;				/* SP On/Off Non-Clip funtion D4SS1_NCLIP		*/
		stInfo.bSpNg_Rat=0; 			/* SP Detection Level of headphone Amplifier's Noise GateD4SS1_SP_NG_RAT	*/	
		stInfo.bSpNg_Att=0; 			/* SP Attenuation Level for headphone Amplifier's Noise GateD4SS1_SP_NG_ATT */	
		
		stInfo.bSpSvol=0;				/* SP soft volume setting, on(0) / off(1) D4SS1_SP_SVOFF		*/
		stInfo.bSpHiZ=0;				/* SP HiZ state D4SS1_SP_HIZ	*/
		
		stInfo.bSpAtt=g_ampgain[cur_mode].sp_att;				/* SP attenuator D4SS1_SP_ATT	*/
		
		stInfo.bSpMixer_Line1=1;		/* SP mixer LINE1 setting D4SS1_SP_AMIX 	*/
		stInfo.bSpMixer_Line2=0;		/* SP mixer LINE2 setting D4SS1_SP_BMIX */
		D4SS1_PowerOn(&stInfo);
		

#if AMPREG_DEBUG
		D4SS1_ReadRegisterByte( 0x80, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x81, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x82, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x83, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x84, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x85, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x86, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x87, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x88, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
#endif
	}
	else
	{
		pr_info(MODULE_NAME ":speaker off\n");
		D4SS1_PowerOff();
	}
}

void yda173_speaker_onoff_inVoip(int onoff) /* speaker path amp onoff */
{
	D4SS1_SETTING_INFO stInfo;
#if AMPREG_DEBUG	
	unsigned char buf;
#endif
	
	if (onoff)
	{
		#ifdef AMP_TUNING
		if(!amptuningFlag)
		{
			Amp_gain_tuning();
		}
		#endif
		
		pr_info(MODULE_NAME ":speaker_inVoip on\n");

		/* input */
		stInfo.bLine1Gain=g_ampgain[0].in1_gain; 	/* LINE1 Gain Amp D4SS1_VA	*/	
		stInfo.bLine2Gain=g_ampgain[0].in2_gain; 	/* LINE2 Gain Amp D4SS1_VB	*/	
		
		stInfo.bLine1Balance=0; /* LINE1 Single-ended(0) or Differential(1) D4SS1_DIFA*/
		stInfo.bLine2Balance=0; /* LINE2 Single-ended(0) or Differential(1) D4SS1_DIFB*/
		
		stInfo.bLineVolMode=0;		/* How input are use  D4SS1_VOLMODE 	*/
		/*HP*/
		stInfo.bHpCtCancel=0;			/* HP CrossTalk Cancel D4SS1_CT_CAN*/
		stInfo.bHpPpdOff=0; 		/* HP partial powerdownfunction D4SS1_PPDOFF*/
		stInfo.bHpModeSel=0;			/* HP enable sound output within 10ms D4SS1_MODESEL*/		
		stInfo.bHpCpMode=0; 		/* HP charge pump mode setting, 3stage mode(0) / 2stage mode(1) D4SS1_CPMODE*/		
		stInfo.bHpAvddLev=0;			/* HP charge pump AVDD level, 1.65V<=AVDD<2.40V(0) / 2.40V<=AVDD<=2.86V(1) D4SS1_VLEVEL*/		
		
		stInfo.bHpNg_AttackTime=0;		/* HP Noise Gate : attack time D4SS1_NG_ATRT*/
		
		stInfo.bHpNg_Rat=0; 			/* HP Detection Level of headphone Amplifier's Noise GateD4SS1_HP_NG_RAT	*/	
		stInfo.bHpNg_Att=0; 			/* HP Attenuation Level for headphone Amplifier's Noise GateD4SS1_HP_NG_ATT */	
		
		stInfo.bHpSvol=0;				/* HP soft volume setting, on(0) / off(1) D4SS1_HP_SVOFF		*/
		stInfo.bHpHiZ=1;				/* HP Hiz state D4SS1_HP_HIZ*/
		
		stInfo.bHpAtt=g_ampgain[0].hp_att;				/* HP attenuator D4SS1_HP_ATT		*/
				
		stInfo.bHpCh=0; 			/* HP channel, stereo(0)/mono(1) D4SS1_HP_MONO		*/
		stInfo.bHpMixer_Line1=0;		/* HP mixer LINE1 setting D4SS1_HP_AMIX 	*/
		stInfo.bHpMixer_Line2=0;		/* HP mixer LINE2 setting D4SS1_HP_BMIX 	*/
		
		/*SP*/
		stInfo.bSpModeSel=0;			/* SP enable sound output within 10ms D4SS1_MODESEL 	*/
		
		stInfo.bSpDrcMode=0;			/* SP DRC or Non-Clip S function D4SS1_DRC_MODE 	*/
		stInfo.bSpNcpl_AttackTime=0;	/* SP Non-Clip power limiter : attack Time D4SS1_DATRT		*/
		stInfo.bSpNcpl_ReleaseTime=0;	/* SP Non-Clip power limiter : release Time D4SS1_DATRT 	*/
		stInfo.bSpNg_AttackTime=0;		/* SP Noise Gate : attack time D4SS1_NG_ATRT		*/
		
		stInfo.bSpNcpl_PowerLimit=0;	/* SP Non-Clip power limiter : Power Limit D4SS1_DPLT		*/
		
		stInfo.bSpNcpl_PowerLimitEx=0;	/* SP Non-Clip power limiter : Power Limit ExD4SS1_DPLT_EX		*/
		stInfo.bSpNclip=0;				/* SP On/Off Non-Clip funtion D4SS1_NCLIP		*/
		stInfo.bSpNg_Rat=0; 			/* SP Detection Level of headphone Amplifier's Noise GateD4SS1_SP_NG_RAT	*/	
		stInfo.bSpNg_Att=0; 			/* SP Attenuation Level for headphone Amplifier's Noise GateD4SS1_SP_NG_ATT */	
		
		stInfo.bSpSvol=0;				/* SP soft volume setting, on(0) / off(1) D4SS1_SP_SVOFF		*/
		stInfo.bSpHiZ=0;				/* SP HiZ state D4SS1_SP_HIZ	*/
		
		stInfo.bSpAtt=g_ampgain[cur_mode].sp_att;				/* SP attenuator D4SS1_SP_ATT	*/
		
		stInfo.bSpMixer_Line1=1;		/* SP mixer LINE1 setting D4SS1_SP_AMIX 	*/
		stInfo.bSpMixer_Line2=0;		/* SP mixer LINE2 setting D4SS1_SP_BMIX */
		D4SS1_PowerOn(&stInfo);

#if AMPREG_DEBUG
		D4SS1_ReadRegisterByte( 0x80, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x81, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x82, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x83, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x84, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x85, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x86, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x87, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x88, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
#endif
	}
	else
	{
		pr_info(MODULE_NAME ":speaker_inVoip off\n");
		D4SS1_PowerOff();
	}
}



void yda173_headset_onoff(int onoff) /* headset path amp onoff */
{
	D4SS1_SETTING_INFO stInfo;
	
	if (onoff)
	{

		#ifdef AMP_TUNING
		if(!amptuningFlag)
		{
			Amp_gain_tuning();
		}
		#endif
    
		pr_info(MODULE_NAME ":headset on\n");

		/* input */
		stInfo.bLine1Gain=g_ampgain[cur_mode].in1_gain; 	/* LINE1 Gain Amp D4SS1_VA	*/	
		stInfo.bLine2Gain=g_ampgain[cur_mode].in2_gain; 	/* LINE2 Gain Amp D4SS1_VB	*/	
		
		stInfo.bLine1Balance=0; /* LINE1 Single-ended(0) or Differential(1) D4SS1_DIFA*/
		stInfo.bLine2Balance=0; /* LINE2 Single-ended(0) or Differential(1) D4SS1_DIFB*/
		
		stInfo.bLineVolMode=0;		/* How input are use  D4SS1_VOLMODE 	*/
		/*HP*/
		stInfo.bHpCtCancel=0;			/* HP CrossTalk Cancel D4SS1_CT_CAN*/
		stInfo.bHpPpdOff=0; 		/* HP partial powerdownfunction D4SS1_PPDOFF*/
		stInfo.bHpModeSel=0;			/* HP enable sound output within 10ms D4SS1_MODESEL*/		
		stInfo.bHpCpMode=0; 		/* HP charge pump mode setting, 3stage mode(0) / 2stage mode(1) D4SS1_CPMODE*/		
		stInfo.bHpAvddLev=0;			/* HP charge pump AVDD level, 1.65V<=AVDD<2.40V(0) / 2.40V<=AVDD<=2.86V(1) D4SS1_VLEVEL*/		
		
		stInfo.bHpNg_AttackTime=0;		/* HP Noise Gate : attack time D4SS1_NG_ATRT*/
		
		stInfo.bHpNg_Rat=0; 			/* HP Detection Level of headphone Amplifier's Noise GateD4SS1_HP_NG_RAT	*/	
		stInfo.bHpNg_Att=0; 			/* HP Attenuation Level for headphone Amplifier's Noise GateD4SS1_HP_NG_ATT */	
		
		stInfo.bHpSvol=0;				/* HP soft volume setting, on(0) / off(1) D4SS1_HP_SVOFF		*/
		stInfo.bHpHiZ=1;				/* HP Hiz state D4SS1_HP_HIZ*/
		
		stInfo.bHpAtt=g_ampgain[cur_mode].hp_att;				/* HP attenuator D4SS1_HP_ATT		*/
				
		stInfo.bHpCh=0; 			/* HP channel, stereo(0)/mono(1) D4SS1_HP_MONO		*/
		stInfo.bHpMixer_Line1=0;		/* HP mixer LINE1 setting D4SS1_HP_AMIX 	*/
		stInfo.bHpMixer_Line2=1;		/* HP mixer LINE2 setting D4SS1_HP_BMIX 	*/
		
		/*SP*/
		stInfo.bSpModeSel=0;			/* SP enable sound output within 10ms D4SS1_MODESEL 	*/
		
		stInfo.bSpDrcMode=0;			/* SP DRC or Non-Clip S function D4SS1_DRC_MODE 	*/
		stInfo.bSpNcpl_AttackTime=0;	/* SP Non-Clip power limiter : attack Time D4SS1_DATRT		*/
		stInfo.bSpNcpl_ReleaseTime=0;	/* SP Non-Clip power limiter : release Time D4SS1_DATRT 	*/
		stInfo.bSpNg_AttackTime=0;		/* SP Noise Gate : attack time D4SS1_NG_ATRT		*/
		
		stInfo.bSpNcpl_PowerLimit=0;	/* SP Non-Clip power limiter : Power Limit D4SS1_DPLT		*/
		
		stInfo.bSpNcpl_PowerLimitEx=0;	/* SP Non-Clip power limiter : Power Limit ExD4SS1_DPLT_EX		*/
		stInfo.bSpNclip=0;				/* SP On/Off Non-Clip funtion D4SS1_NCLIP		*/
		stInfo.bSpNg_Rat=0; 			/* SP Detection Level of headphone Amplifier's Noise GateD4SS1_SP_NG_RAT	*/	
		stInfo.bSpNg_Att=0; 			/* SP Attenuation Level for headphone Amplifier's Noise GateD4SS1_SP_NG_ATT */	
		
		stInfo.bSpSvol=0;				/* SP soft volume setting, on(0) / off(1) D4SS1_SP_SVOFF		*/
		stInfo.bSpHiZ=0;				/* SP HiZ state D4SS1_SP_HIZ	*/
		
		stInfo.bSpAtt=g_ampgain[cur_mode].sp_att;				/* SP attenuator D4SS1_SP_ATT	*/
		
		stInfo.bSpMixer_Line1=0;		/* SP mixer LINE1 setting D4SS1_SP_AMIX 	*/
		stInfo.bSpMixer_Line2=0;		/* SP mixer LINE2 setting D4SS1_SP_BMIX */
		D4SS1_PowerOn(&stInfo);
#if AMPREG_DEBUG
		D4SS1_ReadRegisterByte( 0x80, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x81, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x82, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x83, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x84, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x85, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x86, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x87, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x88, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
#endif
	}
	else
	{
		pr_info(MODULE_NAME ":headset off\n");
		D4SS1_PowerOff();
	}
}
void yda173_speaker_headset_onoff(int onoff) /* speaker+headset path amp onoff */
{
	D4SS1_SETTING_INFO stInfo;
	
	if (onoff)
	{
		pr_info(MODULE_NAME ":speaker+headset on\n");
		/* input */
		stInfo.bLine1Gain=g_ampgain[cur_mode].in1_gain; 	/* LINE1 Gain Amp D4SS1_VA	*/	
		stInfo.bLine2Gain=g_ampgain[cur_mode].in2_gain; 	/* LINE2 Gain Amp D4SS1_VB	*/	
		
		stInfo.bLine1Balance=0; /* LINE1 Single-ended(0) or Differential(1) D4SS1_DIFA*/
		stInfo.bLine2Balance=0; /* LINE2 Single-ended(0) or Differential(1) D4SS1_DIFB*/
		
		stInfo.bLineVolMode=0;		/* How input are use  D4SS1_VOLMODE 	*/
		/*HP*/
		stInfo.bHpCtCancel=0;			/* HP CrossTalk Cancel D4SS1_CT_CAN*/
		stInfo.bHpPpdOff=0; 		/* HP partial powerdownfunction D4SS1_PPDOFF*/
		stInfo.bHpModeSel=0;			/* HP enable sound output within 10ms D4SS1_MODESEL*/		
		stInfo.bHpCpMode=0; 		/* HP charge pump mode setting, 3stage mode(0) / 2stage mode(1) D4SS1_CPMODE*/		
		stInfo.bHpAvddLev=0;			/* HP charge pump AVDD level, 1.65V<=AVDD<2.40V(0) / 2.40V<=AVDD<=2.86V(1) D4SS1_VLEVEL*/		
		
		stInfo.bHpNg_AttackTime=0;		/* HP Noise Gate : attack time D4SS1_NG_ATRT*/
		
		stInfo.bHpNg_Rat=0; 			/* HP Detection Level of headphone Amplifier's Noise GateD4SS1_HP_NG_RAT	*/	
		stInfo.bHpNg_Att=0; 			/* HP Attenuation Level for headphone Amplifier's Noise GateD4SS1_HP_NG_ATT */	
		
		stInfo.bHpSvol=0;				/* HP soft volume setting, on(0) / off(1) D4SS1_HP_SVOFF		*/
		stInfo.bHpHiZ=1;				/* HP Hiz state D4SS1_HP_HIZ*/
		
		stInfo.bHpAtt=g_ampgain[cur_mode].hp_att;				/* HP attenuator D4SS1_HP_ATT		*/
				
		stInfo.bHpCh=0; 			/* HP channel, stereo(0)/mono(1) D4SS1_HP_MONO		*/
		stInfo.bHpMixer_Line1=0;		/* HP mixer LINE1 setting D4SS1_HP_AMIX 	*/
		stInfo.bHpMixer_Line2=1;		/* HP mixer LINE2 setting D4SS1_HP_BMIX 	*/
		
		/*SP*/
		stInfo.bSpModeSel=0;			/* SP enable sound output within 10ms D4SS1_MODESEL 	*/
		
		stInfo.bSpDrcMode=0;			/* SP DRC or Non-Clip S function D4SS1_DRC_MODE 	*/
		stInfo.bSpNcpl_AttackTime=0;	/* SP Non-Clip power limiter : attack Time D4SS1_DATRT		*/
		stInfo.bSpNcpl_ReleaseTime=0;	/* SP Non-Clip power limiter : release Time D4SS1_DATRT 	*/
		stInfo.bSpNg_AttackTime=0;		/* SP Noise Gate : attack time D4SS1_NG_ATRT		*/
		
		stInfo.bSpNcpl_PowerLimit=0;	/* SP Non-Clip power limiter : Power Limit D4SS1_DPLT		*/
		
		stInfo.bSpNcpl_PowerLimitEx=0;	/* SP Non-Clip power limiter : Power Limit ExD4SS1_DPLT_EX		*/
		stInfo.bSpNclip=0;				/* SP On/Off Non-Clip funtion D4SS1_NCLIP		*/
		stInfo.bSpNg_Rat=0; 			/* SP Detection Level of headphone Amplifier's Noise GateD4SS1_SP_NG_RAT	*/	
		stInfo.bSpNg_Att=0; 			/* SP Attenuation Level for headphone Amplifier's Noise GateD4SS1_SP_NG_ATT */	
		
		stInfo.bSpSvol=0;				/* SP soft volume setting, on(0) / off(1) D4SS1_SP_SVOFF		*/
		stInfo.bSpHiZ=0;				/* SP HiZ state D4SS1_SP_HIZ	*/
		
		stInfo.bSpAtt=g_ampgain[cur_mode].sp_att;				/* SP attenuator D4SS1_SP_ATT	*/
		
		stInfo.bSpMixer_Line1=1;		/* SP mixer LINE1 setting D4SS1_SP_AMIX 	*/
		stInfo.bSpMixer_Line2=0;		/* SP mixer LINE2 setting D4SS1_SP_BMIX */
		D4SS1_PowerOn(&stInfo);
#if AMPREG_DEBUG
		D4SS1_ReadRegisterByte( 0x80, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x81, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x82, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x83, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x84, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x85, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x86, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x87, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
		D4SS1_ReadRegisterByte( 0x88, &buf);
		pr_info(MODULE_NAME ":%d = %02x\n",__LINE__,buf);
#endif
	}
	else
	{
		pr_info(MODULE_NAME ":speaker+headset off\n");
		D4SS1_PowerOff();
	}
}
void yda173_tty_onoff(int onoff) /* tty path amp onoff */
{
	D4SS1_SETTING_INFO stInfo;
	
	if (onoff)
	{
		pr_info(MODULE_NAME ":tty on\n");

		/* input */
		stInfo.bLine1Gain=2; 	/* LINE1 Gain Amp D4SS1_VA	*/	
		stInfo.bLine2Gain=2; 	/* LINE2 Gain Amp D4SS1_VB	*/	
		
		stInfo.bLine1Balance=0; /* LINE1 Single-ended(0) or Differential(1) D4SS1_DIFA*/
		stInfo.bLine2Balance=1; /* LINE2 Single-ended(0) or Differential(1) D4SS1_DIFB*/
		
		stInfo.bLineVolMode=0;		/* How input are use  D4SS1_VOLMODE 	*/
		/*HP*/
		stInfo.bHpCtCancel=0;			/* HP CrossTalk Cancel D4SS1_CT_CAN*/
		stInfo.bHpPpdOff=0; 		/* HP partial powerdownfunction D4SS1_PPDOFF*/
		stInfo.bHpModeSel=0;			/* HP enable sound output within 10ms D4SS1_MODESEL*/		
		stInfo.bHpCpMode=0; 		/* HP charge pump mode setting, 3stage mode(0) / 2stage mode(1) D4SS1_CPMODE*/		
		stInfo.bHpAvddLev=0;			/* HP charge pump AVDD level, 1.65V<=AVDD<2.40V(0) / 2.40V<=AVDD<=2.86V(1) D4SS1_VLEVEL*/		
		
		stInfo.bHpNg_AttackTime=0;		/* HP Noise Gate : attack time D4SS1_NG_ATRT*/
		
		stInfo.bHpNg_Rat=0; 			/* HP Detection Level of headphone Amplifier's Noise GateD4SS1_HP_NG_RAT	*/	
		stInfo.bHpNg_Att=0; 			/* HP Attenuation Level for headphone Amplifier's Noise GateD4SS1_HP_NG_ATT */	
		
		stInfo.bHpSvol=0;				/* HP soft volume setting, on(0) / off(1) D4SS1_HP_SVOFF		*/
		stInfo.bHpHiZ=1;				/* HP Hiz state D4SS1_HP_HIZ*/
		
		stInfo.bHpAtt=127;				/* HP attenuator D4SS1_HP_ATT		*/
				
		stInfo.bHpCh=0; 			/* HP channel, stereo(0)/mono(1) D4SS1_HP_MONO		*/
		stInfo.bHpMixer_Line1=0;		/* HP mixer LINE1 setting D4SS1_HP_AMIX 	*/
		stInfo.bHpMixer_Line2=1;		/* HP mixer LINE2 setting D4SS1_HP_BMIX 	*/
		
		/*SP*/
		stInfo.bSpModeSel=0;			/* SP enable sound output within 10ms D4SS1_MODESEL 	*/
		
		stInfo.bSpDrcMode=0;			/* SP DRC or Non-Clip S function D4SS1_DRC_MODE 	*/
		stInfo.bSpNcpl_AttackTime=0;	/* SP Non-Clip power limiter : attack Time D4SS1_DATRT		*/
		stInfo.bSpNcpl_ReleaseTime=0;	/* SP Non-Clip power limiter : release Time D4SS1_DATRT 	*/
		stInfo.bSpNg_AttackTime=0;		/* SP Noise Gate : attack time D4SS1_NG_ATRT		*/
		
		stInfo.bSpNcpl_PowerLimit=0;	/* SP Non-Clip power limiter : Power Limit D4SS1_DPLT		*/
		
		stInfo.bSpNcpl_PowerLimitEx=0;	/* SP Non-Clip power limiter : Power Limit ExD4SS1_DPLT_EX		*/
		stInfo.bSpNclip=0;				/* SP On/Off Non-Clip funtion D4SS1_NCLIP		*/
		stInfo.bSpNg_Rat=0; 			/* SP Detection Level of headphone Amplifier's Noise GateD4SS1_SP_NG_RAT	*/	
		stInfo.bSpNg_Att=0; 			/* SP Attenuation Level for headphone Amplifier's Noise GateD4SS1_SP_NG_ATT */	
		
		stInfo.bSpSvol=0;				/* SP soft volume setting, on(0) / off(1) D4SS1_SP_SVOFF		*/
		stInfo.bSpHiZ=0;				/* SP HiZ state D4SS1_SP_HIZ	*/
		
		stInfo.bSpAtt=0;				/* SP attenuator D4SS1_SP_ATT	*/
		
		stInfo.bSpMixer_Line1=0;		/* SP mixer LINE1 setting D4SS1_SP_AMIX 	*/
		stInfo.bSpMixer_Line2=0;		/* SP mixer LINE2 setting D4SS1_SP_BMIX */
		D4SS1_PowerOn(&stInfo);
	}
	else
	{
		pr_info(MODULE_NAME ":tty off\n");
		D4SS1_PowerOff();
	}
}

static int amp_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int amp_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int amp_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	/* int mode;
	 * switch (cmd)
	 * {
		 * case SND_SET_AMPGAIN :
			 * if (copy_from_user(&mode, (void __user *) arg, sizeof(mode)))
			 * {
				 * pr_err(MODULE_NAME ": %s fail\n", __func__);
				 * break;
			 * }
			 * if (mode >= 0 && mode < MODE_NUM_MAX)
				 * cur_mode = mode;

			 * break;
		 * default :
			 * break;
	 * } */
	return 0;
}

static struct file_operations amp_fops = {
	.owner = THIS_MODULE,
	.open = amp_open,
	.release = amp_release,
	.ioctl = amp_ioctl,
};

static struct miscdevice amp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "amp",
	.fops = &amp_fops,
};

static int yda173_probe(struct i2c_client *client, const struct i2c_device_id * dev_id)
{
	int err = 0;
	
	pr_info(MODULE_NAME ":%s\n",__func__);
	

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		goto exit_check_functionality_failed;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		return err;
	}

	pclient = client;

	memcpy(&g_data, client->dev.platform_data, sizeof(struct yda173_i2c_data));

	if(misc_register(&amp_device))
	{
		pr_err(MODULE_NAME ": misc device register failed\n"); 		
	}
	load_ampgain();

	err = sysfs_create_group(&amp_device.this_device->kobj, &yda173_attribute_group);
	if(err)
	{
		pr_err(MODULE_NAME ":sysfs_create_group failed %s\n", amp_device.name);
		goto exit_sysfs_create_group_failed;
	}

	if(charging_boot) //shim
		D4SS1_PowerOff();

	return 0;

exit_check_functionality_failed:
exit_sysfs_create_group_failed:
	return err;
}

static int yda173_remove(struct i2c_client *client)
{
	pclient = NULL;

	return 0;
}


static const struct i2c_device_id yda173_id[] = {
	{ "yda173", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, yda173_id);

static struct i2c_driver yda173_driver = {
	.id_table   	= yda173_id,
	.probe 			= yda173_probe,
	.remove 		= yda173_remove,
/*
#ifndef CONFIG_ANDROID_POWER
	.suspend    	= yda173_suspend,
	.resume 		= yda173_resume,
#endif
	.shutdown 		= yda173_shutdown,
*/
	.driver = {
		.name   = "yda173",
	},
};

static int __init yda173_init(void)
{
	pr_info(MODULE_NAME ":%s\n",__func__);
	return i2c_add_driver(&yda173_driver);
}

static void __exit yda173_exit(void)
{
	i2c_del_driver(&yda173_driver);
}

module_init(yda173_init);
module_exit(yda173_exit);

MODULE_AUTHOR("Jongcheol Park");
MODULE_DESCRIPTION("YDA173 Driver");
MODULE_LICENSE("GPL");



