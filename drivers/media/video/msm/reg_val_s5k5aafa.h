//============================================================================
//
//   Register Setting Data File for 
//   SYSTEMLSI 1.3 Mega Pixel CMOS YCbCr sensor
//   Set  File version 10.06.21

const unsigned short MCLK_TABLE[1] = {
// ------------------------------------
// 필요한 클럭 하나만 주석 풀고 사용
// ------------------------------------
//  6144   // 6.144MHz Jitter free
// 12288  // 12.288MHz Jitter Free
// 19200  // TCXO
// 24000  // 24.0MHz
24576      // 24.576MHz Jitter Free
//25000   // 25.0MHz
// 32000  // 32.0MHz
// 34000  // 34.0MHz
// 34560  // 34.56MHz
// 48000  // 48.0MHz
// 49152  // 49.152MHz Jitter free
};

const unsigned short INIT_DATA[] ={ 
// ================================================== 
// 	caMeRa INITIaL                                
// ================================================== 
0xfc02,
0x3100, 
0x3204, 
     
0xfc00, 
0x00aa, 
0xfc07,
0x6601, 
0xfc01, 
0x0401, 
     
0xfc07, 
0x0900,
0x0ab3,
0x0b00,
0x0ca3,
0x0e40, 
0xfc00,
0x7042,   
//==================================================
//	Clock setting
//==================================================
0xfc07,
0x3705, // Main clock precision
0xfc00,
0x7278, // 24Mhz 

0xfc02, // Frame Start line
0x1000,
0x1122,
0x1203,
0x13ec,

// ================================================== 
// 	CIS setting                                   
// ================================================== 
0xfc02,
0x2e82,  // analog offset(default 80)              
0x2f00,   // b channel line adlc tuning register    
0x3000,  // gb channel line adlc tuning register   
0x3718,  // Global Gain(default 18)                
0x44df,   // cLP_eN on                              
0x45f0,   // 1R eNd,1S eNd time(ff)                 
0x470f,   //  3_S4 adrs                              
0x4a30,  // clamp                                  
0x4b11,  // Ramp current                           
0x4d02,  // dbLR Tune                              
0x4e2d,  // SCL_SDA_CTRL_ADRS                      
0x4faa,   // noise check       
0x551f,   // channel Line adLc on                   
0x5700,  // R channel line adlc tuning register    
0x5800,  // gr channel line adlc tuning register   
0x5b0f,   // Ob_Ref                                 
0x6001,  // ST_en                                  
0x620d,  // lew 03                                 
0x6322,  // cdS aMP current                        
0x2db8,  //double Shutter                         
0x3100,  // CFPN Off   

// ==================================================      
//	Shutter Suppress                                   
// ==================================================     
0xfc00,
0x8300,  //Shutter off,Double shutter off 
0xfc0b,
0x5c60,
0x5d60,     
        
// ==================================================
// 	Table Set for capture                         
// ==================================================
0xfc03, 
0x0204, 
0x0305, //capture 8fps
0x0e0e,                
0x120a,               
0x2804,             
0x2900,              
0x2e03, // CrCb_sel 

// ================================================== 
// 	Table Set for Preview                         
// ================================================== 
0xfc04,   
0x9202, 
0x9339,  //20 Preview 15~7.5fps 
0x9e06, // 06                  
0xa206, // 06                  
0xbe03, // CrCb_sel  

// ==================================================
// 	COMMAND setting                               
// ==================================================
0xfc00,           
0x034b,                                                   
0x2904,  //  Brightness_H                                    
0x2a00,  //  Brightness_L                                    
0x2b04,  //  color level                                    
0x2c00,  //  color level  
0x3202,  //  AWB_Average_Num                                     
0x620a,  // Control Enable  Ahn Test (mirror??)	     
0x6c98,  // AE target_L            
0x6d00,  // AE target_H                                         
0x7311,  //  Frame AE                                            
0x7418,  //  flicker 60Hz fix : 18                            
0x786a,  //  AGC Max                               
0x8190,  //  Mirror X shading on, Ggain offset off, EIT GAMMA ON 
0x0700,  //  EIT gamma Th.                                       
0x0820,  //  EIT gamma Th.
                                    
0xfc20,                                                
0x010a,  //  Stepless_Off	new shutter                          
0x0212,  //  Flicker Dgain Mode	                             
0x0302,                                                 
0x1001,  // shutter Max                                        
0x11a0,  //  for new LED spec (40ms = 0170h)	             
0x1650,  // Frame AE Start (Gain 2.9 =54h)             
0x2501,  //  Cintr Min	                                         
0x2ca0,  //  Forbidden cintc	                                 
0x550a,  // agc1                                              
0x560a,  // agc2                                              
0x570c,  // Stable_Frame_AE

// ================================================== 
// 	ISP setting                                   
// ================================================== 
0xfc01, 
0x0100, // Pclk inversion  
0x0c02, 
   
// =================================================  
// 	color Matrix                                  
//=================================================  
0xfc01,
0x5107,
0x5295, 
0x53fe,
0x5420,
0x55fe,
0x564b,

0x57ff, 
0x5830,
0x5906,
0x5abb,
0x5bfe,
0x5c3d,

0x5dff,
0x5e55,
0x5ffd,
0x608b,
0x6107, 
0x6200,

// ================================================== 
// 	edge enhancement                                                  
// ================================================== 
0xfc00,
0x8903,  // Edge Mode on     
0xfc0b,
0x4250,  // edge aGc MIN               
0x4360,  // edge aGc MaX               
0x451a,  // positive gain aGc MIN     
0x4908,  // positive gain aGc MaX 
0x4d18,  // negative gain aGc MIN     
0x5108,  // negative gain aGc MaX 
0xfc05,
0x3420, // YaPTcLP:Y edge clip                            
0x3518, // YaPTSc:Y edge Noiselice 10->8                     
0x360b, // eNHaNce                                             
0x3f00,  // NON-LIN                                             
0x4041, // Y delay                                             
0x4210, // eGfaLL:edge coloruppress Gainlope               
0x4300, // HLfaLL:High-light coloruppress Gainlope         
0x45a0, // eGRef:edge coloruppress Reference Thres.          
0x467a, // HLRef:High-light coloruppress Reference Thres.    
0x4740, // LLRef:Low-light coloruppress Reference Thres.     
0x480c, // [5:4]edge,[3:2]High-light,[1:0]Low-light            
0x4931, // cSSeL  eGSeL  cS_dLY  
      
// ================================================= 
// 	gamma setting                                
// ================================================= 
0xfc0c,   // outdoor Gamma
0x000a,  //R
0x0125,  
0x0258,  
0x03ba, 
0x0400,
     
0x0566, 
0x0600, 
0x0777, 
0x08c0, 
0x096a, 
     
0x0af8,
0x0b28,
0x0c50,
0x0d70,
0x0ebf,
     
0x0f90,
0x10a4,
0x11ba,
0x12ce,
0x13ff,
     
0x14de,
0x15ef,
0x16ff,
0x17fc,
  	 
0x180a,  //G
0x1925,
0x1a58,
0x1bca, 
0x1C00,   	     
     
0x1d80,   	     
0x1e10,   	     
0x1f78,   	     
0x20c0,   	     
0x216a,   	     
     
0x22f8,   	     
0x2328,   	     
0x2450,   	     
0x2570,   	     
0x26bf,   	     
     
0x2790,   	     
0x28a4,   	     
0x29ba,   	     
0x2ace,   	     
0x2bff,   	     
     
0x2cde,   	     
0x2def,   	     
0x2eff,   	     
0x2ffc,   	     
     
0x300a,  //B
0x3125,  
0x3258,  
0x33ca,  
0x3400,   	
     
0x3580,	
0x3610, 	
0x3778,   	
0x38c0,   	
0x396a,   	
     
0x3af8,   	
0x3b28,   	
0x3c50,   	
0x3d70,   	
0x3ebf,   	
     
0x3f90,   	
0x40a4,   	
0x41ba,   	
0x42ce,   	
0x43ff,   	
     
0x44de,   	
0x45ef,   	
0x46ff,   	
0x47fc,        

0xfc0d,  // indoor Gamma
0x0015, 
0x0130, 
0x026a, 
0x03ba,
0x0400,
     
0x055e,
0x06fc, 
0x0768,  
0x08b2,       
0x095a,   	     
     
0x0af4,   	     
0x0b2a,   	     
0x0c5a,   	     
0x0d7e,   	     
0x0ebf,  	     
     
0x0f9c,   	     
0x10b6,   	     
0x11ca,   	     
0x12d8,   	     
0x13ff,   	     
     
0x14e6,   	     
0x15f0,   	     
0x16ff,   	     
0x17fc,   	     
     
0x1815,  //G  
0x1930,  
0x1a6a,
0x1bba,  
0x1c00, 
     
0x1d5e, 
0x1efc,  
0x1f68,   	     
0x20b2,   	     
0x215a,   	     
     
0x22f4,   	     
0x232a,   	     
0x245a,   	     
0x257e,   	     
0x26bf,   	     
     
0x279c,   	     
0x28b6,   	     
0x29ca,   	     
0x2ad8,   	     
0x2bff,   	     
     
0x2ce6,   	     
0x2df0,   	     
0x2eff,   	     
0x2ffc,   	     
     
0x3015,  //B
0x3130,  
0x326a,
0x33ba, 
0x3400,
 	 
0x355e,  
0x36fc,   	
0x3768,  
0x38b2,   	
0x395a,   	
     
0x3af4,   	
0x3b2a,   	
0x3c5a,   	
0x3d7e,   	
0x3ebf,   	
     
0x3f9c,   	
0x40b6,   	
0x41ca,   	
0x42d8,   	
0x43ff,   	
     
0x44e6,   	
0x45f0,   	
0x46ff,   	
0x47fc,  
          
// =================================================        
// 	Hue setting                                         
// =================================================        
0xfc00,               
0x4835, //30
0x4930,
0x4af2, 
0x4b04,
0x4c30,
0x4d40, 
0x4eec,
0x4ffb, 
       
0x503a, 
0x5150,  
0x52f3, 
0x5320, 
0x5440, 
0x5558, //5a  60  
0x5608, 
0x5700, 
                            
0x583e,
0x595a,
0x5a13, //11
0x5bf0,
0x5c35,
0x5d32, 
0x5e0f,
0x5ff4, //f6

// =================================================  
// 	suppress functions                            
// =================================================  
0xfc00,   
0x7ef4,  // [7]:BPR on[6]:NR on[5]:CLPF on[4]:GrGb on 
             // [2]:Cgain on[1]:Ygain-on[0]:D_Clamp         
// =================================================  
// 	bPR                                           
// =================================================  
0xfc01,         
0x3d10,   // PBPR On              
0xfc0b,
0x0b00,   // ISP BPR On Start     
0x0c40,   //Th13 AGC Min         
0x0d50,   //Th13 AGC Max 
0x0e00,   // Th1 Max H for AGCMIN 
0x0f20,    // Th1 Max L for AGCMIN 
0x1000,   // Th1 Min H for AGCMAX 
0x1110,   // Th1 Min L for AGCMAX 
0x1200,   // Th3 Max H for AGCMIN 
0x137f,    // Th3 Max L for AGCMIN 
0x1403,   // Th3 Min H for AGCMAX 
0x15ff,     // Th3 Min L for AGCMAX 
0x1648,   // Th57 AGC Min         
0x1760,   // Th57 AGC Max         
0x1800,   // Th5 Max H for AGCMIN 
0x1900,   // Th5 Max L for AGCMIN 
0x1a00,   // Th5 Min H for AGCMAX 
0x1b20,   // Th5 Min L for AGCMAX 
0x1c00,   // Th7 Max H for AGCMIN 
0x1d00,   // Th7 Max L for AGCMIN 
0x1e00,   // Th7 Min H for AGCMAX 
0x1f20,    // Th7 Min L for AGCMAX 

// ================================================= 
// 	NR                                           
// ================================================= 
0xfc01,   
0x4c01, // NR Enable                                  
0x4914, // Sig_Th Mult                              
0x4b0a, // Pre_Th Mult      
                  
0xfc0b, 
0x2800, // NR start AGC     
0x2900, // SIG Th AGCMIN H  
0x2a14, // SIG Th AGCMIN L  
0x2b00, // SIG Th AGCMAX H  
0x2c14, // SIG Th AGCMAX L  
0x2d00, // PRE Th AGCMIN H  
0x2e80, // c0PRE Th AGCMIN L                          
0x2f00,  // PRE Th AGCMAX H  
0x30c0, // e0 PRE Th AGCMAX L                           
0x3100, // POST Th AGCMIN H 
0x3280, // 90 POST Th AGCMIN L                         
0x3300, // POST Th AGCMAX H 
0x34c0, // POST Th AGCMAX L 

// ================================================== 
// 	1d-Y/c-SIGMa-LPF                      
// ================================================== 
0xfc01,
0x05C0,

0xfc0b,
0x3500,  // YLPF start AGC        
0x3620,  // YLPF01 AGCMIN       
0x3750,  // YLPF01 AGCMAX      
0x3800,  // YLPF SIG01 Th AGCMINH 
0x3920,  // 10 YLPF SIG01 Th AGCMINL 
0x3a00,  // YLPF SIG01 Th AGCMAXH 
0x3b50,  // YLPF SIG01 Th AGCMAXH 
0x3c30,  // YLPF02 AGCMIN         
0x3d50,  // YLPF02 AGCMAX        
0x3e00,  // YLPF SIG02 Th AGCMINH 
0x3f1e,   // YLPF SIG02 Th AGCMINL 
0x4000,  // YLPF SIG02 Th AGCMAXH 
0x4150,  // YLPF SIG02 Th AGCMAXH 
0xd430,  // CLPF AGCMIN    0404       
0xd550,  // CLPF AGCMAX    0404  
0xd6b0,  // CLPF SIG01 Th AGCMIN  
0xd7f0,   // CLPF SIG01 Th AGCMAX  
0xd8b0,  // CLPF SIG02 Th AGCMIN  
0xd9f0,   // CLPF SIG02 Th AGCMAX  

// ================================================== 
// 	GR/Gb CORRECTION                              
// ================================================== 
0xfc01,   
0x450c,

0xfc0b,
0x2100, // Start AGC    
0x2230, // AGCMIN 
0x2350, // AGCMAX 
0x2412, // G Th AGCMIN
0x2520, // G Th AGCMAX 
0x2612, // RB Th AGCMIN
0x2720, // RB Th AGCMAX

//==================================================  
//	color Suppress                                
//==================================================  
0xfc0b,
0x0850,	
0x0903,	
0x0aa0,
   
// ==================================================  
// 	Shading                                        
// ==================================================  
0xfc09,
// DsP9_sH_WIDTH_H 
0x0105,
0x0200,
// DsP9_sH_HEIGHT_H 
0x0304,
0x0400,
// DsP9_sH_XCH_R 
0x0502,
0x067a,
0x0702,
0x0805,
// DsP9_sH_XCH_G 
0x0902,
0x0A79,
0x0B01, 
0x0Cf3, 
// DsP9_sH_XCH_B 
0x0D02,
0x0E63, 
0x0F01,
0x10c1,
// DsP9_sH_Del_eH_R 
0x1D10,
0x1E52, 
0x1F10,
0x2000,
0x2310,
0x2427,
0x210f, 
0x2288, 
// DsP9_sH_Del_eH_G 
0x250F,
0x26D6,
0x2710,
0x2800,
0x2B10,
0x2C00,
0x2910,
0x2A00,
// DsP9_sH_Del_eH_B 
0x2D10,
0x2E00,
0x2F0F,
0x3061,
0x330F,
0x343b,
0x3110,
0x3200,
// DsP9_sH_VAL_R0H 
0x3501,
0x3602,
0x3701,
0x380e,
0x3901,
0x3A36,
0x3B01,
0x3C73,
0x3D01,
0x3E95,
0x3F01,
0x40bb,
0x4101,
0x42ea, 
0x4302,
0x4419,

// DsP9_sH_VAL_G0H 
0x4501,
0x4600,
0x4701,
0x4808,
0x4901,
0x4A22,
0x4B01,
0x4C4B,
0x4D01,
0x4E62,
0x4F01,
0x507F,
0x5101,
0x52A0,
0x5301,
0x54C9,
// DsP9_sH_VAL_B0H 
0x5501,
0x5600,
0x5701,
0x5806,
0x5901,
0x5A1e,
0x5B01,
0x5C3d,
0x5D01,
0x5E4c,
0x5F01,
0x6067,
0x6101,
0x6289,
0x6301,
0x64ad,
// DsP9_sH_M_R2_R1H 
0x6500,
0x665f,
0x67c4,
0x6801,
0x697f,
0x6A11,
0x6B03,
0x6C5d,
0x6De6,
0x6E04,
0x6F95,
0x7023,
0x7105,
0x72fc,
0x7343,
0x7407,
0x7593,
0x7645,
0x7709,
0x785a,
0x7929,
// DsP9_sH_M_R2_G1H 
0x7A00,
0x7B59,
0x7C22,
0x7D01,
0x7E64,
0x7F87,
0x8003,
0x8122,
0x8230,
0x8304,
0x8443,
0x85DE,
0x8605,
0x8792,
0x881D,
0x8907,
0x8A0C,
0x8BEC,
0x8C08,
0x8DB4,
0x8E4D,
// DsP9_sH_M_R2_B1H 
0x8F00,
0x905A,
0x9167,
0x9201,
0x9369,
0x949E,
0x9503,
0x962D,
0x97A3,
0x9804,
0x9953,
0x9A73,
0x9B05,
0x9CA6,
0x9D76,
0x9E07,
0x9F26,
0xA0AE,
0xA108,
0xA2D4,
0xA319,
// DsP9_sH_sUB_RR0H 
0xA4Ab,
0xA515,
0xA639,
0xA707,
0xA822,
0xA937,
0xAA34,
0xABa4,
0xAC2d,
0xAD9f,
0xAE28,
0xAF41,
0xB024,
0xB104,
// DsP9_sH_sUB_RG0H 
0xB2B7,
0xB3D1,
0xB43D,
0xB545,
0xB624,
0xB7C3,
0xB838,
0xB98F,
0xBA31,
0xBB04,
0xBC2B,
0xBD40,
0xBE26,
0xBFB2,
// DsP9_sH_sUB_RB0H 
0xC0B5,
0xC13B,
0xC23C,
0xC369,
0xC424,
0xC53F,
0xC637,
0xC7C3,
0xC830,
0xC954,
0xCA2A,
0xCBA4,
0xCC26,
0xCD27,
0x0002,  // shading on                    
                                                                                                              
// --------------------  
//  X-Shading            
// --------------------  
0xfc1B,
0x4900,
0x4A41,
0x4B00,
0x4C6C,
0x4D03,
0x4EF0,
0x4F00,
0x5009,
0x517B,
0x5800,
0x59B6,
0x5A01,
0x5B29,
0x5C01,
0x5D73,
0x5E01,
0x5F91,
0x6001,
0x6192,
0x6201,
0x635F,
0x6400,
0x65EB,
0x6600,
0x673A,
0x6800,
0x6973,
0x6A00,
0x6B9A,
0x6C00,
0x6DAC,
0x6E00,
0x6FB2,
0x7000,
0x71A1,
0x7200,
0x7370,
0x7407,
0x75E2,
0x7607,
0x77E4,
0x7807,
0x79E0,
0x7A07,
0x7BE1,
0x7C07,
0x7DE3,
0x7E07,
0x7FEB,
0x8007,
0x81EE,
0x8207,
0x837E,
0x8407,
0x8548,
0x8607,
0x8718,
0x8807,
0x8900,
0x8A06,
0x8BFB,
0x8C07,
0x8D1B,
0x8E07,
0x8F58,
0x9007,
0x9113,
0x9206,
0x93B2,
0x9406,
0x9569,
0x9606,
0x974A,
0x9806,
0x9946,
0x9A06,
0x9B73,
0x9C06,
0x9DDC,

0x4801, // x-shading on       

//====================================================
//	AE Window Weight                              
//====================================================
0xfc20,                                 
0x1c00,

0xfc06,
0x0140,
0x0398,
0x0548,
0x079A,
0x0910,
0x0b27,
0x0d20,
0x0f50,
0x313a,
0x3349,
0x3548,
0x3746,
0x3800,
0x390a,
0x3a00,
0x3b13,
0x3c00,
0x3d10,
0x3e00,
0x3f28,

0xfc20,
0x540a,             
0x6011,
0x6111,
0x6211,
0x6311,
0x6411,
0x6511,
0x6611,
0x6711,
0x6811,
0x6912,
0x6a21,
0x6b11,
0x6c11,
0x6d12,
0x6e21,
0x6f11,
0x7022,
0x7122,
0x7221,
0x7311,
0x7411,
0x7511,
0x7611,
0x7711,    

// ================================================== //
// 	SaIT aWb         //                             
// ================================================== //
// ==================================================//
// 	aWb table Margin    //                          
// ================================================== //
0xfc00,
0x8d03,

// =================================================//
// 	aWb Offset setting //                         
// =================================================//
0xfc00,
0x79ee,
0x7a09, 
0xfc07,
0x1100,  
           
//====================================================//
//	AWB Gain Offset   //                        
//====================================================//
0xfc22,
0x5801,   // D65 R Offset     
0x5903,   // D65 B Offset   
  
0x5A04,   // 5000K R Offset  
0x5B09,   // 5000K B Offset   

0x5C09,   // CWF R Offset     
0x5D0a,   //0b 0e CWF B Offset     

0x5E0b,   // 3000K R Offset   
0x5F0b,   //0c 0d 10 3000K B Offset 

0x600c,    // Incand A R Offset
0x6102,    //03 04 Incand A B Offset 

0x621b,    // 2000K R Offset    
0x63fe,      //ff 00 2000K B Offset   

//====================================================//
//	AWB Basic setting      //                       
//====================================================//
0xfc01,            
0xced0, // AWB Y Max  
0xfc00,            
0x3d04, // AWB Y_min Low    
0x3e10, // AWB Y_min_Normal 
0xbcf0,
0xfc05,                    
0x6400, // Darkslice R 
0x6500, // Darkslice G 
0x6600, // Darkslice B 

//====================================================
// AWB ETC ; recommand after basic coef.              
//====================================================
0xfc00,
0x8b05,
0x8c05, 

0xfc22,
0xde00,  // LARGE OBJECT BUG FIX  
0x70f0,   // Green stablizer ratio 
0xd4f0,   // Low temperature       
0x9012,
0x9112,             
0x9807,  // Moving Equation Weight 
0xfc22,   // Y up/down threshold    
0x8c07,	
0x8d07,
0x8e03, 
0x8f05,
0xfc07,
0x6ba0, // AWB Y Max  
0x6c08, // AWB Y_min  
0xfc00, 
0x3206, // AWB moving average 8 frame  
 
//====================================================//
//	White Point      //                                                                                                            
//====================================================//
0xfc22,                                                                                                                                 
0x01d0,  // D65       
0x03a1,  

0x05c1,  // 5100K     
0x07bd,  

0x09b3,  // CWF       
0x0bd9,  

0x0da0,  //  Incand A 
0x0e00,  
0x0fea,  

0x1190,  // 3100K    
0x1200,  
0x13fa,  

0x1583, //  Horizon  
0x1600,  
0x17ff,   
                                                                                                                                          
//====================================================
//	Basic setting                                                                                                                  
//====================================================
0xfc22,                                                                                                                       
0xA001,                                                                                                                                 
0xA120,                                                                                                                                 
0xA20F,                                                                                                                                 
0xA3D8,                                                                                                                                 
0xA407,                                                                                                                                 
0xA5FF,                                                                                                                                 
0xA610,                                                                                                                                 
0xA728,                                                                                                                                 
0xA901,                                                                                                                                 
0xAADB,                                                                                                                                 
0xAB0E,                                                                                                                                 
0xAC1D,                                                                                                                                 
0xAD02,                                                                                                                                 
0xAEBA,                                                                                                                                 
0xAF2C,                                                                                                                                 
0xB0B6,                                                                                                                                 
0x9437,                                                                                                                                 
0x9533,                                                                                                                                 
0x9658,                                                                                                                                 
0x9757,                                                                                                                                 
0x6710,                                                                                                                                 
0x6801,                                                                                                                                 
0xD056,                                                                                                                                 
0xD134,                                                                                                                                 
0xD265,                                                                                                                                 
0xD31A,                                                                                                                                 
0xD4B7,
0xDB34,                                                                                                                                 
0xDC00,                                                                                                                                 
0xDD1A,                                                                                                                                 
0xE700,                                                                                                                                 
0xE8C5,                                                                                                                                 
0xE900,                                                                                                                                 
0xEA63,                                                                                                                                 
0xEB05,                                                                                                                                 
0xEC3D,                                                                                                                                 
0xEE78,                                                                                                                               
                                                                                                                                          
//====================================================
//	Pixel Filter Setting                          
//====================================================

0xfc01,
0xd94d,  //R max
0xda90, 
0xdb3e,  //R min
0xdc10, 
0xdd63,  //B max
0xde00,  
0xdf51,   //B min
0xe080,  

0xe126,
0xe209,
0xe31f,
0xe4d0,
0xe51e,
0xe6b1,
0xe71a,
0xe873,
0xe940,
0xea40,
0xeb3f,
0xec40,
0xed36,
0xee22,
0xef2f,
0xf023,
0xf100,
                                                                                                                                          
//====================================================
//	Polygon AWB Region Tune                                                                                                
//====================================================                     
//AWB H 광원 개선
0xfc22,
0x1800,
0x1974,
0x1adb,
0x1b00,
0x1c89,
0x1db8,
0x1e00,
0x1f92,
0x20a2,
0x2100,
0x22a7,
0x2389,
0x2400,
0x25bd,
0x267b,
0x2700,
0x28d5,
0x2970,
0x2a00,
0x2be9,
0x2c67,
0x2d00,
0x2edf,
0x2f5d,
0x3000,
0x31b8,
0x326b,
0x3300,
0x3496,
0x357d,
0x3600,
0x376c,
0x38aa,
0x3900,
0x3a52,
0x3bc5,
0x3c00,
0x3d5e,
0x3eee,
0x3f00,
0x4000,
0x4100,
0x4200,
0x4300,
0x4400,
0x4500,
0x4600,
0x4700,
0x4800,
0x4900,
0x4a00,
0x4b00,
0x4c00,
0x4d00,
0x4e00,
0x4f00,
0x5000,
0x5100,
0x5200,
0x5300,
0x5400,
0x5500,
0x5600,
                                                                                                                                          
//====================================================
//	EIT Threshold                                
//====================================================
0xfc22,                                                                                     
0xb100,  //sunny                                                                                                                       
0xb203,                                                                                                                      
0xb30d,                                                                                                                         
0xb449,                                                                                                       
0xb500,  //Cloudy영역줄임                                                                                                              
0xb603,                                                                                                                            
0xb70d,                                                                                                                              
0xb94a,                                                                                                           
0xd7ff,    // large object                                                                                                             
0xd8ff,                                                                                                   
0xd9ff,                                                                                                   
0xdaff,                                                                                                         
                                                                                                                                          
//====================================================
//	Aux Window set                                                                                                                
//====================================================
0xfc22,
0x7a00,
0x7b00,
0x7cc0,
0x7d70,
0x7e0e,
0x7f00,
0x80aa,
0x8180,
0x8208,
0x8300,
0x84c0,
0x8570,
0x8608,
0x8700,
0x88c0,
0x8970,
0x8a0e,
                                                                                                                                          
//==================================================
//	AWB Option                                                                                                                   
//==================================================
0xfc22,                                                                                                                                 
0xbd84,  // outdoor classify enable, White Option enable, Green stablilizer enable 
                                         
//==================================================
//	Special Effect                              
//==================================================
0xfc07,	
0x30c0, // Sepia Cr    
0x3120, // Sepia Cb    
0x3240, // Aqua Cr     
0x33c0, // Aqua Cb     
0x3400, // Green Cr    
0x35b0, // Green Cb    
0xfc00, 
0x0208, // Mode Change 
0x7b01,
};

const unsigned short OP_PREVIEW_DATA[] = {

0xfc01,
0x0c02,     

0xfc20,
0x1480,

0xfc00,
0x0208,  // 640 x 480 
0x034b,  // AE/AWB ON 
0x7ef4,   // Suppress ON 
0x8903,  // edge Supress ON 

};

const unsigned short OP_LOWLIGHT_PREVIEW_DATA[] = {

0xfc00,
0x0208,   // 640 x 480 
0x034b,  // AE/AWB ON 
0x786a,  // AGC Gain
0x7ef4,   // Suppress ON 
0x8903,  // edge Supress ON 
    
0xfc00,
0x2b03, //color level H     
0x2c20, //color level L          
0x79ed, //AWB R Gain offset        
0x7a0d, //AWB B Gain offset   

0xfc09, //Shading

// DsP9_sH_VAL_R0H 
0x3501,
0x3606,
0x3701,
0x3811,
0x3901,
0x3A34,
0x3B01,
0x3C6c,
0x3D01,
0x3E86,
0x3F01,
0x40ac,
0x4101,
0x42da,
0x4302, 
0x440c,

0xfc05,
0x3420, // YaPTcLP:Y edge clip                    
0x3518, // YaPTSc:Y edge Noiselice  

0xfc0d, // indoor Gamma
0x0000, //R
0x0128,  
0x0261,	   
0x03c1,   
0x0400,

0x1800,  //G  
0x1928, 
0x1a61, 
0x1bc1,
0x1c00, 

0x3000, //B
0x3128, 
0x3261,
0x33c1,
0x3400,

//  GR/GB CORRECTION 
0xfc0b,
0x2414, // G Th AGCMIN    
0x2525, // G Th AGCMAX    
0x2614, // RB Th AGCMIN  
0x2725, // RB Th AGCMAX   

//  NR  
0xfc0b,
0x2a10,   // SIG Th AGCMIN L   
0x2c14,   // SIG Th AGCMAX L   
0x2ea0,   // PRE Th AGCMIN L   
0x2f01,    // PRE Th AGCMAX H   
0x30a0,   // PRE Th AGCMAX L   
0x3100,   // POST Th AGCMIN H  
0x32a0,   // POST Th AGCMIN L  
0x3301,   // POST Th AGCMAX H  
0x3410,   // POST Th AGCMAX L  

};

const unsigned short OP_MIDDLELIGHT_PREVIEW_DATA[] = {

0xfc00,
0x0208,    // 640 x 480 
0x034b,   // AE/AWB ON 
0x786a,   // AGC Gain
0x7ef4,    // Suppress ON 
0x8903,   // edge Supress ON 

0xfc09,   //Shading
              // DsP9_sH_VAL_R0H 
0x3501,
0x3606, 
0x3701,
0x3811,
0x3901,
0x3A34,
0x3B01,
0x3C6c,
0x3D01,
0x3E86,
0x3F01,
0x40ac,
0x4101,
0x42da,
0x4302, 
0x440c,

0xfc00,
0x79ee, 
0x7a09,

0xfc00,
0x2b04,  //color level H     
0x2c00,  //color level L  

0xfc05,
0x3430,  // YaPTcLP:Y edge clip                            
0x3514,  // YaPTSc:Y edge Noiselice 10->8 

};

const unsigned short OP_NORMAL_PREVIEW_DATA[] = {       

0xfc00,
0x0208,   // 640 x 480 
0x034b,  // AW/AWB ON 
0x786a,  // AGC Gain
0x6ca5,  // AE target_L            
0x6d00,  //  AE target_H  
0x7ef4,   // Suppress ON 
0x8903,  // edge Supress ON 

0xfc09, //Shading

// DsP9_sH_VAL_R0H 
0x3501,
0x3602,
0x3701,
0x380e,
0x3901,
0x3A36,
0x3B01,
0x3C73,
0x3D01,
0x3E95,
0x3F01,
0x40bb,
0x4101,
0x42ea,
0x4302,
0x4419,

0xfc00,
0x2b04, //color level H     
0x2c00, //color level L   

0xfc00,      
0x79e9,   //ea ed AWB R Gain offset     
0x7a0c,   //0c 0a AWB B Gain offset     

0xfc05,
0x3420, //YaPTcLP:Y edge clip                    
0x3518, //YaPTSc:Y edge Noiselice 10->8 

0xfc00,
// Hue    
0x503e,
0x514a,
0x5200,
0x5300,
0x543b,
0x553a,
0x5600,
0x57fd,
     
0x583d,
0x5950,
0x5a06,
0x5bf8,
0x5c40,
0x5d3d,
0x5e1b, 
0x5ffd,

0xfc00,
0x034b,

};

const unsigned short OP_SNAPSHOT_DATA[]  = {

0xfc00,
0x0348,
0x6ca5,   //AE target_L            
0x6d00,   //AE target_H  
0x79e9,   //AWB R Gain offset     
0x7a0c,   //AWB B Gain offset 

0x503e,
0x514a,
0x5200,
0x5300,
0x543b,
0x553a,
0x5600,
0x57fd,
     
0x583d,
0x5950,
0x5a06,
0x5bf8,
0x5c40,
0x5d3d,
0x5e1b,
0x5ffd,

0xfc02,
0x3726,  

0xfc20,
0x146c,   

0xfc00,
0x2f00,
0x0200,
0x7301,

};

const unsigned short OP_MIDDLELIGHT_SNAPSHOT_DATA[]  = {

0xfc00,
0x0348,

0xfc02,
0x3725,

0xfc20,
0x1466, 

0xfc00,
0x2f00,
0x0200,
0x7301,

};

const unsigned short OP_LOWLIGHT_SNAPSHOT_DATA[]  = {

0xfc01,
0x0c00,	   
        
0xfc00,  
0x0348,           
0x7e00,
0x8900,

0xfc02,
0x3725,

0xfc20,  //brightness offset              
0x1439,

0xfc0d,  //indoor  
0x0020, 	     
0x0140,	  	   
0x0278,	  	   
0x03d0,	  	   
0x0400,   	
 
0x0564,   	     
0x06e8,   	     
0x0758,   	     
0x08a8,   	     
0x095a,   	     
 
0x0ae0,   	     
0x0b10,   	     
0x0c38,   	     
0x0d58,   	     
0x0ebf,   	     
 
0x0f78,   	     
0x1094,   	     
0x11aa,   	     
0x12c0,   	     
0x13ff,   	     
 
0x14d8,   	     
0x15ea,   	     
0x16ff,   	     
0x17fc,   	     

0x1820,	  //g   
0x1940,	 
0x1a78,	 
0x1bd0,	 
0x1c00,   	     

0x1d64,   	     
0x1ee8,   	     
0x1f58,   	     
0x20a8,   	     
0x215a,   	     

0x22e0,   	     
0x2310,   	     
0x2438,   	     
0x2558,   	     
0x26bf,   	     

0x2778,   	     
0x2894,   	     
0x29aa,   	     
0x2ac0,   	     
0x2bff,   	     

0x2cd8,   	     
0x2dea,   	     
0x2eff,   	     
0x2ffc,   	     

0x3020,   //b   
0x3140,  
0x3278,  
0x33d0,  
0x3400,  
 	
0x3564,   	
0x36e8,   	
0x3758,   	
0x38a8,   	
0x395a,   	

0x3ae0,   	
0x3b10,   	
0x3c38,   	
0x3d58,   	
0x3ebf,   	

0x3f78,   	
0x4094,   	
0x41aa,   	
0x42c0,   	
0x43ff,   	

0x44d8,   	
0x45ea,   	
0x46ff,   	
0x47fc,   	  	 	  	  

0xfc01,
0x3e00,   //BPR1 thres
0x46c0,   //GrGb thres
0x47c0,   

0xfc05, 
0x7700,  // YLPF 01 low th
0x7800,  // YLPF 01 high th
0x7905,  // high
0x2c20,  // Y edge posi
0x3020,  // Y edge nega
     
0xfc02,  
0x3c10,     // normal bpr th
0x028a,     // CIS BPR 

0xfc01,  
0x3e00,     // BPR Th1
0x3fff,    
0x4003, 
0x41ff,   
0x42ff,   
0x4300, 
0x4400, 

0xfc00,
0x0200,
0x7301,	

};

const unsigned short OP_NIGHTSHOT_SNAPSHOT_DATA[]  = {

0xfc00,
0x0200,

0xfc03, 
0x0005, 
0x01B2, 
0x020F,  
0x0350,  //2fps 

0xfc00,
0x786a,

0x2905, 
0x2a60,

0xfc02,
0x2d00,
0x373f, 

0x470f,
0x45f0,

0xfc0b,   //2010 02 09 LSI
0x2f02,   //00 NR_PRE_H
0x3000,  //c0 NR_PRE_L
0x3302,  //00 NR_POST_L
0x3400,  //c0 NR_POST_L
0x5000,  //00 NEG Gain_H_H
0x5110,  //08 NEG Gain_H_L
0x5210,  //00 NEG Gain_L_H
0x5310,  //00 NEG Gain_L_L

0xfc20,
0x1420, 

0xfc00,
0x7ef4, 
0x8903, 

0x2f00,
0x0349, 

0xfc00,
0x7301,  // frame AE
};

//======================================
// AWB setting
//======================================
const unsigned short WB_AUTO_DATA[] = {

0xfc00,
0x0400,
0x0500,

0xfc22,
0x0400,
0x05c1, 

0x07bd, 

0xfc00,
0x3000, 

};

const unsigned short WB_TUNGSTEN_DATA[] = {
0xfc00,
0x0400,
0x0500,

0xfc22,
0x0400,
0x0595,

0x07ea,

0xfc00,
0x3002,
};      
        
const unsigned short WB_FLUORESCENT_DATA[] = {
0xfc00,
0x0400,
0x0500,

0xfc22,
0x0400,
0x05c9,

0x07d3,

0xfc00,
0x3002,
};

const unsigned short WB_SUNNY_DATA[] = {
0xfc00,
0x0400,
0x0500,

0xfc22,
0x0400,
0x05e1,

0x0792,

0xfc00,
0x3002,
};

const unsigned short WB_CLOUDY_DATA[] = {
0xfc00,
0x0400,
0x0500,

0xfc22,
0x0401, 
0x0508,

0x0783, //85

0xfc00,
0x3002,
};

//-------------------------------------------------------------------------
// Usage     : Effect 
//=========================================================================
const unsigned short EFFECT_NORMAL_DATA[] = {
0xfc00,
0x2500,
};

const unsigned short EFFECT_B_AND_W_DATA[] = {
0xfc00,
0x2508,
};

const unsigned short EFFECT_NEGATIVE_DATA[] = {
0xfc00,
0x2504,
};

const unsigned short EFFECT_SEPIA_DATA[] = {
0xfc00,
0x2501,
};

const unsigned short EFFECT_GREEN_DATA[] = {
0xfc00,
0x2510,
};

const unsigned short EFFECT_AQUA_DATA[] = {
0xfc00,
0x2502,
};

const unsigned short METERING_AVERAGE_DATA[] = {
0xfc20,
0x540a, // AE Stable      
0x6011,
0x6111,
0x6211,
0x6311,
0x6411,
0x6511,
0x6611,
0x6711,
0x6811,
0x6911,
0x6a11,
0x6b11,
0x6c11,
0x6d11,
0x6e11,
0x6f11,
0x7011,
0x7111,
0x7211,
0x7311,
0x7411,
0x7511,
0x7611,
0x7711,
};

const unsigned short METERING_CENTER_DATA[] = {

0xfc20,      //AE window weight for Center mode 
0x6011,
0x6111,
0x6211,
0x6311,
0x6411,
0x6511,
0x6611,
0x6711,
0x6811,
0x6912,
0x6a21,
0x6b11,
0x6c11,
0x6d12,
0x6e21,
0x6f11,
0x7022,
0x7122,
0x7221,
0x7311,
0x7411,
0x7511,
0x7611,
0x7711,
};      
        
const unsigned short METERING_SPOT_DATA[] = {
0xfc20, 
0x6000,  //Spot mode
0x6100, 
0x6200, 
0x6300, 
0x6401, 
0x6511, 
0x6611, 
0x6710, 
0x6801, 
0x69ff, 
0x6aff, 
0x6b10, 
0x6c01, 
0x6dff,
0x6eff,
0x6f10,
0x7001,
0x7111,
0x7211,
0x7310,
0x7400,
0x7500,
0x7600,
0x7700,
};

const unsigned short FLIP_NONE_DATA[] = {
0xfc00,
};

const unsigned short FLIP_WATER_DATA[] = {
0xfc00,
};

const unsigned short FLIP_MIRROR_DATA[] = {
0xfc00,
};

const unsigned short FLIP_WATER_MIRROR_DATA[] = {
0xfc00,
};

const unsigned short BRIGHTNESS_1_DATA[] = {
0xfc00,
0x2901,
0x2a50,
};

const unsigned short BRIGHTNESS_2_DATA[] = {
0xfc00,
0x2902,
0x2a00,
};

const unsigned short BRIGHTNESS_3_DATA[] = {
0xfc00,
0x2902,
0x2a70,
};

const unsigned short BRIGHTNESS_4_DATA[] = {
0xfc00,
0x2903,
0x2a40,
};

const unsigned short BRIGHTNESS_5_DATA[] = {
0xfc00,
0x2904,
0x2a00,
};

const unsigned short BRIGHTNESS_6_DATA[] = {
0xfc00,
0x2904,
0x2a80,
};

const unsigned short BRIGHTNESS_7_DATA[] = {
0xfc00,
0x2905,
0x2a00,
};

const unsigned short BRIGHTNESS_8_DATA[] = {
0xfc00,
0x2905,
0x2ad0,
};

const unsigned short BRIGHTNESS_9_DATA[] = {
0xfc00,
0x2906,
0x2a90, 
};

const unsigned short NIGHTSHOT_ON_DATA[] = {

0xfc00,
0x786a,
0x2905,
0x2a00,

0xfc02,  
0x2d00,  // double shutter            
0x373f,   // Global Gain(default 18)   
0x44db, // cLP_eN on               
0x45f0,  // S1R eNd S1S eNd time(ff)  
0x470f,  // S3_S4 adrs                 
0x4a30, // clamp                      
0x4b33,                                  
0x4d02, // dbLR tune                   
0x4f2a,  // IO                         
0x551f,  // channel Line adLc on       
0x5b0f,  // Ob_Ref                    
0x6209,                                  
0x6000, //slew                       
0x6311, 

0xfc03, 
0x0204,  
0x0305,   // 8fps

0xfc01,  //color matrix
0x5104,
0x5200,
0x5300,
0x5400,
0x5500,
0x5600,
0x5700,
0x5800,
0x5904,
0x5a00,
0x5b00,
0x5c00,
0x5d00,
0x5e00,
0x5f00,
0x6000,
0x6104,
0x6200,

0xfc0d,  // indoor Gamma
0x0010, 
0x0140, 
0x0280, 
0x03f5,
0x0400,
     
0x059a,
0x0617, 
0x0778,  
0x08ca,       
0x096a,   	     
     
0x0aff,   	     
0x0b3a,   	     
0x0c6a,   	     
0x0d8e,   	     
0x0ebf,  	     
     
0x0fac,   	     
0x10c6,   	     
0x11da,   	     
0x12e8,   	     
0x13ff,   	     
     
0x14f6,   	     
0x15fa,   	     
0x16ff,   	     
0x17fc,   	     
     
0x1810,  // G  
0x1940,  
0x1a80, 
0x1bf5,  
0x1c00, 
     
0x1d9a, 
0x1e17,  
0x1f78,   	     
0x20ca,   	     
0x216a,   	     
     
0x22ff,   	     
0x233a,   	     
0x246a,   	     
0x258e,   	     
0x26bf,   	     
     
0x27ac,   	     
0x28c6,   	     
0x29da,   	     
0x2ae8,   	     
0x2bff,   	     
     
0x2cf6,   	     
0x2dfa,   	     
0x2eff,   	     
0x2ffc,   	     
     
0x3010,  // B
0x3140,  
0x3280, 
0x33f5, 
0x3400,
 	 
0x359a,  
0x3617,   	
0x3778,  
0x38ca,   	
0x396a,   	
     
0x3aff,   	
0x3b3a,   	
0x3c6a,   	
0x3d8e,   	
0x3ebf,   	
     
0x3fac,   	
0x40c6,   	
0x41da,   	
0x42e8,   	
0x43ff,   	
     
0x44f6,   	
0x45fa,   	
0x46ff,   	
0x47fc,
0xfc00,
0x7331,  // frame AE
0x2508,  //special effect
};

const unsigned short NIGHTSHOT_OFF_DATA[] ={

0xfc00,
0x2500,
0x786a,
0x2904,
0x2a00,
0x2b04,  //color level                                    
0x2c00,  //color level  

0xfc03, 
0x0204,  
0x0305,   // 8fps 

0xfc02,
0x2db8,   // double shutter        
0x3718,   // Global Gain(default 18)   
0x44db,  //  CLP_eN on                
0x45ff,    //  S1R eNd S1S eNd time(ff)  
0x472f,   //  S3_S4 adrs             
0x4a30,  //  clamp                     
0x4b11,                            
0x4d02,  // dbLR tune                
0x4faa,   // IO                       
0x551f,   // channel Line adLc on       
0x5b0f,   // Ob_Ref                    
0x6001,
0x620d,  // slew                      
0x6322,

0xfc01,   //color matrix
0x5107,
0x5295, 
0x53fe,
0x5420,
0x55fe,
0x564b,

0x57ff, 
0x5830,
0x5906,
0x5abb,
0x5bfe,
0x5c3d,

0x5dff,
0x5e55,
0x5ffd,
0x608b,
0x6107, 
0x6200,

0xfc0d,  // indoor Gamma
0x0015, 
0x0130, 
0x026a,
0x03ba,
0x0400,
     
0x055e,
0x06fc, 
0x0768,  
0x08b2,       
0x095a,   	     
     
0x0af4,   	     
0x0b2a,   	     
0x0c5a,   	     
0x0d7e,   	     
0x0ebf,  	     
     
0x0f9c,   	     
0x10b6,   	     
0x11ca,   	     
0x12d8,   	     
0x13ff,   	     
     
0x14e6,   	     
0x15f0,   	     
0x16ff,   	     
0x17fc,   	     
     
0x1815,  //G  
0x1930,  
0x1a6a,
0x1bba,  
0x1c00, 
     
0x1d5e, 
0x1efc,  
0x1f68,   	     
0x20b2,   	     
0x215a,   	     
     
0x22f4,   	     
0x232a,   	     
0x245a,   	     
0x257e,   	     
0x26bf,   	     
     
0x279c,   	     
0x28b6,   	     
0x29ca,   	     
0x2ad8,   	     
0x2bff,   	     
     
0x2ce6,   	     
0x2df0,   	     
0x2eff,   	     
0x2ffc,   	     
     
0x3015,  //B
0x3130,  
0x326a, 
0x33ba, 
0x3400,
 	 
0x355e,  
0x36fc,   	
0x3768,  
0x38b2,   	
0x395a,   	
     
0x3af4,   	
0x3b2a,   	
0x3c5a,   	
0x3d7e,   	
0x3ebf,   	
     
0x3f9c,   	
0x40b6,   	
0x41ca,   	
0x42d8,   	
0x43ff,   	
     
0x44e6,   	
0x45f0,   	
0x46ff,   	
0x47fc,  

0xfc0b,  // 2010 02 09 LSI
0x2f00,
0x30c0,
0x3300,
0x34c0,
0x5000,
0x5108,
0x5200,
0x5300,

0xfc20,
0x1480,

0xfc00,
0x7311,  // frame AE
0x7ef4,
0x8903, 
0x034b, 

};

const unsigned short LOWLIGHT_THRESHOLD_TABLE[] = {
0x0068,
};

const unsigned short MIDDLELIGHT_THRESHOLD_TABLE[] = {
0x0001,
};

const unsigned short AWB_THRESHOLD_TABLE[] = {
0xba,
};

const unsigned short AWB_DATA[] = {

0xfc00,    
0x583e,
0x595a,
0x5a13, //11
0x5bf0,
0x5c35,
0x5d32, 
0x5e0f,
0x5ff4, //f6

};

const unsigned short AWB_NORMAL_DATA[] = {

0xfc00,                
0x583b,
0x596d, 
0x5a03,
0x5bf3,   
0x5c3c, 
0x5d35,
0x5e0a, 
0x5ffe,

};

const unsigned short NIGHTVISION_SNAPSHOT_DELAY_TIME[] = {
1100,   // init capture - svc layer
1500,   // low light capture
1500,   // middle light capture
1500,   // normal light capture 
};

const unsigned short NIGHTVISION_IRED_START_DELAY_TIME[] = {
1100,    // low 
1100,    // normal
};

const unsigned short NIGHTVISION_SNAPSHOT_IRED_PULSE_ON_OFF_TIME[] = {
900,   // ON(ms)
3600, // OFF(ms)
};