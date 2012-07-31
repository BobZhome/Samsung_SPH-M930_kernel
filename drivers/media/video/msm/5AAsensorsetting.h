
// ===================================================
// Name     : 5aafx_eVT2 Module                     
// Version  : v2.0                                    
// PLL mode : 0ff   McLK - 27MHz                   
// fPS      :                                         
// Made by  : TRULY Eric                                 
// date     : 10/05/31                                     
// ===================================================


// CAMIF_SECTION_MAX_ITEMS_1500
const unsigned short reg_init_list[] =
{
0xfc02,
0x3100, // cfPN Off //
0x3204, // cfPNtart frame //

0xfc00,
0x00aa, // for edS check //
0x0208, // change resolution SXGA -> VGA

0xfc07,
0x6601, // WdT on//
0xfc01,
0x0401, // aRM clock divider //

0xfc07, // Initial setting   //
0x0900,
0x0ab3,
0x0b00,
0x0ca3,
0x0e40,

0xfc00,
0x7042,

//==================================================//
//	clock setting                               //
//==================================================//
0xfc07,
0x3700, // Main clock precision //

0xfc00,
0x7287, // flicker for 27MHz -> 27 //

//==================================================//
//	cIS setting                                 //
//==================================================//
0xfc02,
0x2e82, // analog offset(default 80)              //
0x2f00, // b channel line adlc tuning register    //
0x3000, // gb channel line adlc tuning register   //
0x3718, // Global Gain(default 18)                //
0x44db, // cLP_eN on                              //
0x45f0, // 1R eNd,1S eNd time(ff)                 //
0x472f, // 3_S4 adrs                              //
0x4a00, // clamp                                  //
0x4b11, // Ramp current                           //
0x4d02, // dbLR Tune                              //
0x4e2d, // SCL_SDA_CTRL_ADRS                      //
0x4faa, // noise check                            //
0x551f, // channel Line adLc on                   //
0x5700, // R channel line adlc tuning register    //
0x5800, // gr channel line adlc tuning register   //
0x5b0f, // Ob_Ref                                 //
0x6001, // ST_en                                  //
0x620d, // lew 03                                 //
0x6322, // cdS aMP current                        //
0x2d58, // double Shutter                         //
0x3100, // cfPN Off                               //

//==================================================//
//	Shutter suppress                            //
//==================================================//
0xfc00,
0x8306, // shutter off, double shutter off  //

0xfc0b,
0x5c60,
0x5d60,

//==================================================//
//	Table set for capture                       //
//==================================================//
0xfc03, // page 03h                   //
0x0005, // cis_frame_h_width_h        //
0x01b2, // cis_frame_h_width_l        //
0x0204, // cis_frame_v_depth_h        //
0x0386, // cis_frame_v_deoth_l        //
0x0405, // cis_output_h_width_h       //
0x0510, // cis_output_h_width_l       //
0x0604, // cis_output_v_depth_h       //
0x070a, // cis_output_v_depth_l       //
0x0801, // cis_h_even_inc_l           //
0x0901, // cis_h_odd_inc_l            //
0x0a01, // cis_v_even_inc_l           //
0x0b01, // cis_v_odd_inc_l            //
0x0c00, // cis_mirr_etc(average sub)  //
0x0d00, // dsp5_post_hstart_h         //
0x0e0a, // dsp5_post_hstart_l         //
0x0f05, // dsp5_post_hwidth_h         //
0x1000, // dsp5_post_hwidth_l         //
0x1100, // dsp5_post_vstart_h         //
0x120a, // dsp5_post_vstart_l         //
0x1304, // dsp5_post_vheight_h        //
0x1400, // dsp5_post_vheight_l        //
0x1505, // dsp5_img_hsizeh            //
0x1600, // dsp5_img_hsizel            //
0x1704, // dsp5_img_vsizeh            //
0x1800, // dsp5_img_vsizel            //
0x1900, // dsp5_psf                   //
0x1a00, // dsp5_msfx_h                //
0x1b00, // dsp5_msfx_l                //
0x1c00, // dsp5_msfy_h                //
0x1d00, // dsp5_msfy_l                //
0x1e05, // dsp5_dw_hsizeh             //
0x1f00, // dsp5_dw_hsizel             //
0x2004, // dso5_dw_vsizeh             //
0x2100, // dsp5_dw_vsizel             //
0x2200, // dsp5_startxh               //
0x2300, // dsp5_startxl               //
0x2400, // dsp5_startyh               //
0x2500, // dsp5_startyl               //
0x2605, // dsp5_clip_hsizeh           //
0x2700, // dsp5_clip_hsizel           //
0x2804, // dsp5_clip_vsizeh           //
0x2900, // dsp5_clip_vsizel           //
0x2a00, // dsp5_sel_main              //
0x2b00, // dsp5_htermh                //
0x2c05, // dsp5_htermm                //
0x2db2, // dsp5_hterml                //
0x2e02, // dsp1_crcb_sel              //
0x2f00, // ozone                      //

//==================================================//
//	Table set for Preview                       //
//==================================================//
0xfc04, // page 04h                  // 
0x9005, // cis_frame_h_width_h       // 
0x91b2, // cis_frame_h_width_l       // 
0x9202, // cis_frame_v_depth_h       // 
0x937c, // cis_frame_v_deoth_l       // 
0x9402, // cis_output_h_width_h      // 
0x9588, // cis_output_h_width_l      // 
0x9602, // cis_output_v_depth_h      // 
0x9704, // cis_output_v_depth_l      // 
0x9801, // cis_h_even_inc_l          // 
0x9903, // cis_h_odd_inc_l           // 
0x9a01, // cis_v_even_inc_l          // 
0x9b03, // cis_v_odd_inc_l           // 
0x9c08, // cis_mirr_etc(average sub) // 
0x9d00, // dsp5_post_hstart_h        // 
0x9e06, // dsp5_post_hstart_l        // 
0x9f02, // dsp5_post_hwidth_h        // 
0xa080, // dsp5_post_hwidth_l        // 
0xa100, // dsp5_post_vstart_h        // 
0xa204, // dsp5_post_vstart_l        // 
0xa301, // dsp5_post_vheight_h       // 
0xa4e0, // dsp5_post_vheight_l       // 
0xa502, // dsp5_img_hsizeh           // 
0xa680, // dsp5_img_hsizel           // 
0xa702, // dsp5_img_vsizeh           // 
0xa800, // dsp5_img_vsizel           // 
0xa900, // dsp5_psf                  // 
0xaa00, // dsp5_msfx_h               // 
0xab00, // dsp5_msfx_l               // 
0xac00, // dsp5_msfy_h               // 
0xad00, // dsp5_msfy_l               // 
0xae02, // dsp5_dw_hsizeh            // 
0xaf80, // dsp5_dw_hsizel            // 
0xb002, // dso5_dw_vsizeh            // 
0xb100, // dsp5_dw_vsizel            // 
0xb200, // dsp5_startxh              // 
0xb300, // dsp5_startxl              // 
0xb400, // dsp5_startyh              // 
0xb500, // dsp5_startyl              // 
0xb602, // dsp5_clip_hsizeh          // 
0xb780, // dsp5_clip_hsizel          // 
0xb802, // dsp5_clip_vsizeh          // 
0xb900, // dsp5_clip_vsizel          // 
0xba00, // dsp5_sel_main             // 
0xbb00, // dsp5_htermh               // 
0xbc05, // dsp5_htermm               // 
0xbdb2, // dsp5_hterml               // 
0xbe02, // dsp1_crcb_sel             // 
0xbf30, // ozone                     // 

//==================================================//
//	cOMMaNd setting                             //
//==================================================//
0xfc00,
0x2904, //03 brightness_H //
0x2a00, //6f 6f brightness_L //
0x2b04, //03 color level H      //
0x2c00, //90 87 color level L   //
0x3202, //02 aWb_average_Num    //
0x620a, //0a Hue control enable   ahn Test (mirror??)   //
0x6ca0, //9a 89 a0 ae target_L  //
0x6d00, //00 ae target_H        //
0x7311, //11 frame ae           //
0x7418, //14 flicker 50Hz auto  //
0x786A, //6a aGc Max            //
0x8190, //90 10 Mirror Xhading on, Ggain offset off, eIT GaMMa ON //
0x0700, //00 eIT gamma Th.	//
0x082d, //20 eIT gamma Th.	//

0xfc20,
0x010a, // Stepless_Off	new shutter //
0x0206, // flicker dgain Mode //
0x0302,
0x1001, // 2 shutter Max 35ms //
0x1148, // 70 35ms for new Led spec (50hz->35ms) // 
0x1480, // brightness offset//
0x1650, // 4c 2.65 frame ae Start (Gain 2 =40h) //
0x2501, // cintr Min //
0x2ca0, // forbidden cintc //
0x5508,
0x5608,
0x570a, // Stable_frame_ae //

//==================================================//
//	ISP setting                                 //
//==================================================//
0xfc01,
0x0101, // pclk inversion//         
0x0c02, // full Yc On //            
            
//==================================================//
//	color Matrix                                //
//==================================================//
0xfc01,
0x5108,
0x523c,
0x53fa,
0x54c2,
0x5501,
0x5602,
0x57fe,
0x58eb,
0x5906,
0x5a5d,
0x5bfe,
0x5cb9,
0x5dff,
0x5ee4,
0x5ffd,
0x6010,
0x6107,
0x620c,

//==================================================//
//	edge enhancement                            //
//==================================================//
0xfc00,
0x8903, // edge Mode on //
0xfc0b,
0x4250, // edge aGc MIN //
0x4360, // edge aGc MaX //
0x451e, // positive gain aGc MIN //
0x490c, // positive gain aGc MaX //
0x4d1e, // negative gain aGc MIN //
0x510c, // negative gain aGc MaX //       

0xfc05,
0x3440, // YaPTcLP:Y edge clip //
0x3518, // YaPTSc:Y edge Noiselice 10->8 //
0x360b, //0b eNHaNce      //
0x3f00, // NON-LIN        //
0x4041, // Y delay        //
0x4210, // eGfaLL:edge coloruppress Gainlope        //
0x4300, // HLfaLL:High-light coloruppress Gainlope  //
0x45a0, // eGRef:edge coloruppress Reference Thres.       //
0x467a, // HLRef:High-light coloruppress Reference Thres. //
0x4740, // LLRef:Low-light coloruppress Reference Thres.  //
0x480c, // [5:4]edge,[3:2]High-light,[1:0]Low-light   //
0x4931, // cSSeL  eGSeL  cS_dLY  //

//==================================================//
//	Gamma setting                               //
//==================================================//
0xfc0c, // outdoor gamma //
      
0x0008,  //04 R gamma// 
0x0118,  //0f// 
0x0248,  //3a//   
0x03b5,  //ba// 
0x0400,  //00// 
          
0x056a,  //6d// 
0x06f0,  //e5// 
0x0750, 
0x0899, 
0x095a,
          
0x0ad3, 
0x0bfe, 
0x0c24, 
0x0d43, 
0x0eaf, 
          
0x0f5d, 
0x1074, 
0x1187, 
0x1295, 
0x13ff, 
          
0x14a2, 
0x15af, 
0x16bd, 
0x17fc,
        
0x1808,  // G gamma //
0x1918,  
0x1a48,  
0x1bb5,  
0x1c00,  
          
0x1d6a,  
0x1ef0, 
0x1f50, 
0x2099, 
0x215a, 
          
0x22d3, 
0x23fe, 
0x2424, 
0x2543, 
0x26af,
          
0x275d, 
0x2874, 
0x2987, 
0x2a95, 
0x2bff,
         
0x2ca2, 
0x2daf, 
0x2ebd, 
0x2ffc,  
        
0x3008,  // B gamma  //
0x3118,  
0x3248,  
0x33b5,  
0x3400,  
          
0x356a,  
0x36f0, 
0x3750, 
0x3899, 
0x395a,
          
0x3ad3,  
0x3bfe, 
0x3c24, 
0x3d43, 
0x3eaf, 
          
0x3f5d, 
0x4074, 
0x4187, 
0x4295, 
0x43ff,
          
0x44a2, 
0x45af, 
0x46bd, 
0x47fc, 
        
0xfc0d,  // Indoor gamma  // 
        
0x000a,  // R gamma       //
0x0120,  
0x0268,  
0x03bf,  
0x0400,   
          
0x0588, 
0x0620, 
0x0780, 
0x08ce, 
0x096a, 
          
0x0a08, 
0x0b32, 
0x0c5c, 
0x0d7e, 
0x0eff, 
          
0x0f9b, 
0x10b4, 
0x11c8, 
0x12d8, 
0x13ff, 
          
0x14e4, 
0x15f0, 
0x16ff, 
0x17fc, 
          
0x180a,   // G gamma //
0x1920, 
0x1a68, 
0x1bbf, 
0x1c00, 
          
0x1d88, 
0x1e20, 
0x1f80, 
0x20ce, 
0x216a,
          
0x2208, 
0x2332, 
0x245c,  
0x257e, 
0x26ff,
          
0x279b, 
0x28b4, 
0x29c8, 
0x2ad8, 
0x2bff, 
          
0x2ce4, 
0x2df0, 
0x2eff, 
0x2ffc,
          
0x300a,   // B gamma  //
0x3120,  
0x3268,  
0x33bf,  
0x3400, 
          
0x3588,  
0x3620,  
0x3780,  
0x38ce,  
0x396a, 
          
0x3a08,  
0x3b32,  
0x3c5c,  
0x3d7e,  
0x3eff, 
          
0x3f9b,  
0x40b4,  
0x41c8,  
0x42d8,  
0x43ff, 
          
0x44e4,  
0x45f0,  
0x46ff,  
0x47fc,  

//=================================================//
//	Hue settng                                 //
//=================================================//
0xfc00,  
0x483e,  // RC 2000K //
0x4940,  //  GC//
0x4afe,  //  RH//
0x4b0a,  //  GH//
0x4c42,  //  BC//
0x4d3e,  //  YC//
0x4ee4,  //  BH//
0x4ff7,  //  YH//
          
0x5038, //  3e RC 3000K //    
0x513d, //  40 GC//          
0x5202, //  fe RH//              
0x53fc, //  0a GH//          
0x543e, //  42 BC//          
0x5552, //  46 YC//          
0x56f4, //  e4 BH//          
0x57ed, //  f7 YH//         
      
0x5838, //  34 RC5100K // 
0x5928, //  28 GC//               
0x5a02, //  08 RH//                
0x5b08, //  10 GH//           
0x5c4d, //  52 BC//                
0x5d38, //  38 YC//               
0x5ee0, //  e0 BH//                    
0x5ffa, //  f7 YH//      

//==================================================//
//	Suppress functions                          //
//==================================================//
0xfc00,
0x7ef4, // [7]:bPR on[6]:NR on[5]:cLPf on[4]:GrGb on //            

//==================================================//
//	bPR                                         //
//==================================================//
0xfc01,
0x3d10, // PbPR On   //

0xfc0b,
0x0b00, // ISP bPR Ontart   //
0x0c40, // Th13 aGc Min   //
0x0d58, // Th13 aGc Max   //
0x0e00, // Th1 Max H for aGcMIN    //
0x0f20, // Th1 Max L for aGcMIN    //
0x1000, // Th1 Min H for aGcMaX   //
0x1110, // Th1 Min L for aGcMaX   //
0x1200, // Th3 Max H for aGcMIN   //
0x137f, // Th3 Max L for aGcMIN   //
0x1403, // Th3 Min H for aGcMaX   //
0x15ff, // Th3 Min L for aGcMaX   //
0x1648, // Th57 aGc Min           //
0x1760, // Th57 aGc Max           //
0x1800, // Th5 Max H for aGcMIN   //
0x1900, // Th5 Max L for aGcMIN   //
0x1a00, // Th5 Min H for aGcMaX   //
0x1b20, // Th5 Min L for aGcMaX   //
0x1c00, // Th7 Max H for aGcMIN   //
0x1d00, // Th7 Max L for aGcMIN   //
0x1e00, // Th7 Min H for aGcMaX   //
0x1f20, // Th7 Min L for aGcMaX   //

//================================================ //
//	NR                                         //
//=================================================//
0xfc01,
0x4c01, // NR enable            //
0x4915, //ig_Th Mult          //
0x4b0a, // Pre_Th Mult          //

0xfc0b,
0x2800, // NR start aGc	     //
0x2900, //IG Th aGcMIN H      //
0x2a14, //IG Th aGcMIN L      //
0x2b00, //IG Th aGcMaX H      //
0x2c14, //IG Th aGcMaX L      //
0x2d00, // PRe Th aGcMIN H      //
0x2e80, // a0 90 PRe Th aGcMIN L   //
0x2f00, // PRe Th aGcMaX H      //
0x30e0, // f0 PRe Th aGcMaX L      //
0x3100, // POST Th aGcMIN H     //
0x3290, // POST Th aGcMIN L    //
0x3300, // POST Th aGcMaX H     //
0x34c0, // POST Th aGcMaX L     //

//=================================================//
//	1d-Y////c-SIGMa-LPf                        //
//=================================================//
0xfc01,
0x05c0,
      
0xfc0b,
0x3500, // YLPftart aGc  //
0x3620, // YLPf01 aGcMIN //
0x3750, // YLPf01 aGcMaX //
0x3800, // YLPfIG01 Th aGcMINH  //
0x3910, // YLPfIG01 Th aGcMINL  //
0x3a00, // YLPfIG01 Th aGcMaXH  //
0x3b30, // YLPfIG01 Th aGcMaXH  //
0x3c40, // YLPf02 aGcMIN        //
0x3d50, // YLPf02 aGcMaX        //
0x3e00, // YLPfIG02 Th aGcMINH  //
0x3f10, // YLPfIG02 Th aGcMINL  //
0x4000, // YLPfIG02 Th aGcMaXH  //
0x4140, // YLPfIG02 Th aGcMaXH  //
0xd440, // cLPf aGcMIN          //
0xd550, // cLPf aGcMaX          //
0xd6b0, // cLPfIG01 Th aGcMIN   //
0xd7f0, // cLPfIG01 Th aGcMaX   //
0xd8b0, // cLPfIG02 Th aGcMIN   //
0xd9f0, // cLPfIG02 Th aGcMaX   //

//==================================================//
//	GR// //Gb cORRecTION                        //
//==================================================//
0xfc01,
0x450c,
0xfc0b,
0x2100, // tart aGc     //
0x2226, // aGcMIN        //
0x2360, // aGcMaX        //
0x2414, // G Th aGcMIN   //
0x2520, // G Th aGcMaX   //
0x2614, // Rb Th aGcMIN  //
0x2720, // Rb Th aGcMaX  //

//==================================================//
//	color suppress                              //
//==================================================//
0xfc0b,
0x0858, // coloruppress aGc MIN  //
0x0904, // coloruppress MIN H    //
0x0a00, // coloruppress MIN L    //

//==================================================//
//	Shading                                     //
//==================================================//
0xfc09,
0x0022,
0x0502, // rx //
0x0678,
0x0702, // ry //
0x0838,  
0x0902, // gx //
0x0a75,
0x0b02, // gy //
0x0c1a,  
0x0d02, // bx //
0x0e58,
0x0f01, // by //
0x10ee, 
0x1d10, // R Right //
0x1e10,  
0x1f0f, // R Left //
0x20e0,  
0x210e, // R Top //
0x22ad,  
0x2311, // R bot //
0x2463,  
0x2510, //10 G Right //
0x2600,  
0x2710, // G Left //
0x2800,  
0x2910, // G Top //
0x2a00,  
0x2b10, // G bot //
0x2c00,  
0x2d10, // b Right //
0x2e00,  
0x2f10, // b Left //
0x3000,  
0x3111, // b Top //
0x3233,  
0x330e, // b bot //
0x34e9,          
0x3501,   
0x3604,   
0x3701,   
0x3814,   
0x3901,   
0x3a41,   
0x3b01,   
0x3c79,  
0x3d01,   
0x3e96,  
0x3f01,   
0x40b3,   
0x4101,   
0x42d8,   
0x4301,   
0x44f2,   
0x4501, // G Gain //
0x4600, 
0x4701, // 2  //
0x480b, 
0x4901, // 3  //
0x4a30, 
0x4b01, // 4  //
0x4c5c, 
0x4d01, // 5  //
0x4e73, 
0x4f01, // 6  //
0x508b, 
0x5101, // 7 //
0x52a5, // aa //
0x5301, // G Gain 8 //
0x54bc, // cf //         
0x5501, // b Gain 1 //
0x5614, // 00 //  
0x5701, // 2  //
0x5821, 
0x5901, // 3  //
0x5a40, // 35, 28 //
0x5b01, // 4  //
0x5c67, // 4e //
0x5d01, // 5 //
0x5e76, // 68 //
0x5f01, // 6 //
0x6089, // 76 //
0x6101, // 7 //
0x629d, // 8f //
0x6301, // b Gain 8 //
0x64b1, // a2 //          
0x6500,
0x665d,
0x6790,
0x6801,
0x6976,
0x6a3f,
0x6b03,
0x6c4a,
0x6d0e,
0x6e04,
0x6f7a,
0x7022,
0x7105,
0x72d8,
0x73fd,
0x7407,
0x7566,
0x76a1,
0x7709,
0x7823,
0x790c,
0x7a00,
0x7b59,
0x7c7e,
0x7d01,
0x7e65,
0x7ff9,
0x8003,
0x8125,
0x8271,
0x8304,
0x8448,
0x854b,
0x8605,
0x8797,
0x88e4,
0x8907,
0x8a14,
0x8b3d,
0x8c08,
0x8dbd,
0x8e55,
0x8f00,
0x9055,
0x91ff,
0x9201,
0x9357,
0x94fd,
0x9503,
0x9605,
0x97fa,
0x9804,
0x991d,
0x9a78,
0x9b05,
0x9c5f,
0x9df6,
0x9e06,
0x9fcd,
0xa073,
0xa108,
0xa265,
0xa3f0,
0xa4af,
0xa51d,
0xa63a,
0xa75f,
0xa823,
0xa905,
0xaa35,
0xabe1,
0xac2e,
0xadb2,
0xae29,
0xaf34,
0xb024,
0xb1dd,
0xb2b7,
0xb313,
0xb43d,
0xb506,
0xb624,
0xb79d,
0xb838,
0xb954,
0xba30,
0xbbd1,
0xbc2b,
0xbd13,
0xbe26,
0xbf8a,
0xc0be,
0xc184,
0xc23f,
0xc381,
0xc426,
0xc51a,
0xc63a,
0xc79e,
0xc832,
0xc9cd,
0xca2c,
0xcbd3,
0xcc28,
0xcd1b,

0x0002,
//==================================================//
//	X-Shading                                   //
//==================================================//
0xfc1b,
0x4900,
0x4a41,
0x4b00,
0x4c6c,
0x4d03,
0x4ef0,
0x4f00,
0x5009,
0x517b,
0x5800,
0x59b6,
0x5a01,
0x5b29,
0x5c01,
0x5d73,
0x5e01,
0x5f91,
0x6001,
0x6192,
0x6201,
0x635f,
0x6400,
0x65eb,
0x6600,
0x673a,
0x6800,
0x6973,
0x6a00,
0x6b9a,
0x6c00,
0x6dac,
0x6e00,
0x6fb2,
0x7000,
0x71a1,
0x7200,
0x7370,
0x7407,
0x75e2,
0x7607,
0x77e4,
0x7807,
0x79e0,
0x7a07,
0x7be1,
0x7c07,
0x7de3,
0x7e07,
0x7feb,
0x8007,
0x81ee,
0x8207,
0x837e,
0x8407,
0x8548,
0x8607,
0x8718,
0x8807,
0x8900,
0x8a06,
0x8bfb,
0x8c07,
0x8d1b,
0x8e07,
0x8f58,
0x9007,
0x9113,
0x9206,
0x93b2,
0x9406,
0x9569,
0x9606,
0x974a,
0x9806,
0x9946,
0x9a06,
0x9b73,
0x9c06,
0x9ddc,

0x4801, // x-shading on //

//==================================================//
//	ae Window Weight                            //
//==================================================//
0xfc20,
0x1c00, // 00=flat01=center02=manual //
      
0xfc06,
0x0140, // SXGa ae Window //
0x0398,
0x0548,
0x079a,
0x0938, // SXGa aWb window //
0x0b26,
0x0d50,
0x0f4e,
0x313a, // ubsampling ae Window //
0x3349,
0x3548,
0x3746,
0x3800, // Subsampling aWb Window //
0x3923,
0x3a00,
0x3b13,
0x3c00,
0x3d20,
0x3e00,
0x3f29,

0xfc20,
0x540a, // ae table //

0x6011,
0x6111,
0x6211,
0x6311,
0x6411,
0x6511,
0x6611,
0x6711,
0x6811,
0x6913,
0x6a31,
0x6b11,
0x6c11,
0x6d13,
0x6e31,
0x6f11,
0x7032,
0x7122,
0x7222,
0x7322,
0x7411,
0x7512,
0x7621,
0x7711,

//==================================================//
//	SaIT aWb                                    //
//==================================================//
//==================================================//
//	aWb table Margin //
//==================================================//
0xfc00,
0x8d03,

//==================================================//
//	AWB Offset setting                          //
//==================================================//
0xfc00,
0x79e9, // aWb R Gain offset //
0x7afd, // aWb b Gain offset //
0xfc07,
0x1100, // aWb G Gain offset //

//==================================================//
//	AWB Gain Offset                             //
//==================================================//
0xfc22,
0x5807, // 02 D65 R Offset //
0x59fc, // fe D65 B Offset //
0x5a03, // 5000K R Offset    //
0x5bfe, // 5000K B Offset    //
0x5c06, // 05 CWF R Offset 02   //
0x5d06, // fe CWF B Offset f2   //
0x5e0c, // 3000K R Offset    //
0x5ffc, // 3000K B Offset    //
0x600a, // Incand A R Offset //
0x61f6, // ec Incand A B Offset //                                      
0x620b, // 10 2000K R Offset  //          
0x63e3, // 2000K B Offset   //      
          
//==================================================//
//	AWB basic setting                           //
//==================================================//
0xfc01,
0xced0, // aWb Y Max   //

0xfc00,
0x3d04, // aWb Y_min Low    //
0x3e10, // aWb Y_min_Normal //
0xfc00,
0x3202, // aWb moving average 8 frame //
0xbcf0,

0xfc05,
0x6400, // darkslice R //
0x6500, // darkslice G //
0x6600, // darkslice b //

//=======================================//
// AWB ETC ; recommand after basic coef. //
//=======================================//
0xfc00,
0x8b05,
0x8c05, // added //
0xfc22,
0xde00, // LaRGe ObJecT bUG fIX //
0x70f0, // Greentablizer ratio //
0xd4f0, // Low temperature //
0x9012,
0x9112,
0x9807, // Moving equation Weight //

0xfc22, // Y up down threshold //
0x8c07,
0x8d07,
0x8e03,
0x8f05,
0xfc07,
0x6ba0, // aWb Y Max //
0x6c08, // aWb Y_min //
0xfc00,
0x3206, // aWb moving average 8 frame  //

//==================================================//
//	White Point                                 //
//==================================================//
0xfc22,
0x01df, // d65  df	//
0x039c, // d65  9c	//        
0x05d0, // 5100K  d0  //
0x07ac, // 5100K  ac  //          
0x09c2, // cWf c2	//
0x0bbf, // cWf bf	//               
0x0db5, //Incand a //
0x0e00,
0x0fcd,          
0x119d, //3100K	//
0x1200,
0x13ed,           
0x158a, // HORizon	//
0x1601,
0x1705,

//==================================================//
//	basic setting                               //
//==================================================//
0xfc22,
0xa001,
0xa12B,
0xa20F,
0xa3D8,
0xa407,
0xa5FF,
0xa610,
0xa76c,
0xa901,
0xaaDB,
0xab0E,
0xac1D,
0xad02,
0xaeBA,
0xaf2C,
0xb0B6,
0x9437,
0x9533,
0x9658,
0x9757,
0x6710,
0x6801,
0xd056,
0xd134,
0xd265,
0xd31A,
0xd4FF,
0xdb34,
0xdc00,
0xdd1A,
0xe700,
0xe8C5,
0xe900,
0xea63,
0xeb05,
0xec3D,
0xee78,

//==================================================//
//	Pixel filteretting                          //
//==================================================//
0xfc01,  
0xd94c, //4c  4c// 
0xda00, //00  00// 
0xdb3a, //3a  3a// 
0xdc00, //00  00// 
0xdd5c, //5c  5c// 
0xde00, //00  00// 
0xdf4c, //48  48// 
0xe000, //00  00// 
0xe123, //23  25// //color tracking //  
0xe26a, //d6  d1//
0xe325, //25  26//
0xe40d, //4c  42//
0xe523, //21  1f//
0xe67f, //e3  d4//
0xe71b, //1a  19//
0xe8c5, //22  bb//
0xe930, //33  37//
0xea40, //40  40//
0xeb3d, //40  40//
0xec40, //40  40//
0xed40, //40  40//
0xee38, //39  3b//
0xef40, //39  35//
0xf025, //21  21//
0xf100, //00  00//

//==================================================//
//	Polygon aWb Region Tune                     //
//==================================================//
0xfc22, 
0x1800, 
0x1977, 
0x1a9a, 
0x1b00, 
0x1c97, 
0x1d80, 
0x1e00, 
0x1fb6, 
0x2068, 
0x2100, 
0x22ce, 
0x2355, 
0x2400, 
0x25d8, 
0x266c, 
0x2700, 
0x28be, 
0x2980, 
0x2a00, 
0x2ba7, 
0x2c94, 
0x2d00, 
0x2e9d, 
0x2fa4, 
0x3000, 
0x3194, 
0x32ca, 
0x3300, 
0x348c, 
0x35ec, 
0x3600, 
0x3742, 
0x38f9, 
0x3900, 
0x3a4d, 
0x3bbd, 

        
//==================================================//
//	eIT Threshold                               //
//==================================================//
0xfc22,
0xb100, // sunny //
0xb203,
0xb300,
0xb449,
0xb500, // cloudy //
0xb603,
0xb700,
0xb850,
0xd7ff, // large object //
0xd8ff,
0xd9ff,
0xdaff,

//==================================================//
//	aux Window set                              //
//==================================================//
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

//==================================================//
//	aWb Option                                  //
//==================================================//
0xfc22,
0xbd84, 

//==================================================//
//	Special effect                              //
//==================================================//
0xfc07,
0x30c0, // epia cr //
0x3120, // epia cb //
0x3240, // aqua cr //
0x33c0, // aqua cb //
0x3400, // Green cr//
0x35b0, // Green cb//

0xfc00,
0x0208,
0x7506, // org - 7500

//---------------------------------------//
// case 21 : Preview                     //
//---------------------------------------//
0xfc00,
0x034b, 
0xfc02,
0x3c00,
0x020e,  // cIS bPR  //

0xfc01,
0x3e15,  // bPR Th1 //
0x3f7f,
0x4000,
0x4100,
0x4200,
0x4300,
0x4400,

};

//-------------------------------------------------------------------------
// Usage     : Preview Mode
//=========================================================================

// caMIf_SecTION_Max_ITeMS_150
const unsigned short reg_preview_camera_list[] =
{
0xfc01, 
0x0c02, // full YC on // 
      
0xfc00, // aWb ON //
0x034b, // aWb ON //
      
0xfc02,        
0x3718, 
0x45f0,           
0x472f,
0x2d58,            
0x6001,    
      
0xfc00,
0x483e, //  3e RC 2000K // 
0x4940, //  40 GC//        
0x4afe, //  fe RH//        
0x4b0a, //  0a GH//        
0x4c42, //  42 BC//        
0x4d3e, //  3e YC//        
0x4ee4, //  e4 BH//        
0x4ff7, //  f7 YH//        
      
0x5038, //  3e RC 3000K //    
0x513d, //  40 GC//          
0x5202, //  fe RH//              
0x53fc, //  0a GH//          
0x543e, //  42 BC//          
0x5552, //  46 YC//          
0x56f4, //  e4 BH//          
0x57ed, //  f7 YH//         
      
0x5838, //  34 RC5100K // 
0x5928, //  28 GC//               
0x5a02, //  08 RH//                
0x5b08, //  10 GH//           
0x5c4d, //  52 BC//                
0x5d38, //  38 YC//               
0x5ee0, //  e0 BH//                    
0x5ffa, //  f7 YH//     
      
0xfc05,
0x6400,
0x6500,
0x6600,         
      
0xfc20,            
0x1480,              
      
0xfc00, 
0x0208, // 640 x512 //
0x7311, // Frame AE //   
0x7ef4, // uppress ON //
0x8306,
0x8903, // edgeupress ON //
      
0xfc02,
0x3c00,
0x020e,   // CIS BPR //
      
0xfc01,
0x3e15,   // BPR Th1 //
0x3f7f,
0x4000,
0x4100,
0x4200,
0x4300,
0x4400,
      
0xfc00,
0x0208,
};

// caMIf_SecTION_Max_ITeMS_150
const unsigned short reg_preview_camcorder_list[] =
{
0xfc02,
0x2d58,
0x3718,
      
0xfc20,
0x1480,
      
0xfc00,
0x7301,
0x8300,
0x0208,

0xff05,
};

//bd07 usys_dmchoi [[
//======================================
//preview 조도
//======================================
const unsigned short reg_preview_high_light_list[] =
{
//hue setting for outdoor//
0xfc00,
0x503e, //  RC 3000K //    
0x5140, //  GC//          
0x52fe, //  RH//              
0x530a, //  GH//          
0x5442, //  BC//          
0x553e, //  YC//          
0x56e4, //  BH//          
0x57f7, //  YH//          
      
0x5830, //  RC5100K // 
0x5948, //  GC//               
0x5af8, //  RH//                
0x5b10, //  GH//           
0x5c52, //  BC//                
0x5d38, //  YC//               
0x5ee0, //  BH//                    
0x5ff0, //  YH//  
};

const unsigned short reg_preview_normal_light_list[] =
{
//hue setting for indoor//
0xfc00,
0x5038, //  3e RC 3000K //    
0x513d, //  40 GC//          
0x5202, //  fe RH//              
0x53fc, //  0a GH//          
0x543e, //  42 BC//          
0x5552, //  46 YC//          
0x56f4, //  e4 BH//          
0x57ed, //  f7 YH//          
      
0x5838, //  34 RC5100K // 
0x5928, //  28 GC//               
0x5a02, //  08 RH//                
0x5b08, //  10 GH//           
0x5c4d, //  52 BC//                
0x5d38, //  38 YC//               
0x5ee0, //  e0 BH//                    
0x5ffa, //  f7 YH// 
};

//======================================
//일반조도 촬영
//======================================
       
// caMIf_SecTION_Max_ITeMS_100
const unsigned short reg_snapshot_list[]  =
{      
0xfc00, 
0x0348, //4B//
0x7301, // Frame AE //   
      
0xfc20,                                   
0x1464, 
      
0xfc02,
0x3725,        
0x470f,
0x2d00,
0x6001,       
      
0xfc00,
0x0200,

0xff21,
};

const unsigned short reg_snapshot_midlight_list[] = 
{
0xfc00,
0x0348,
0x7301, // Frame AE //
      
0xfc20,
0x1475,
      
0xfc02,
0x3725,
0x45f0,
0x470f,
0x2d00,
0x6001,
      
0xfc00,
0x0200, 

0xff21,

};
//bd07 usys_dmchoi ]]
const unsigned short reg_snapshot_mid_lowlight_list[] = 
{
0xfc00,
0x0348,
0x7301, // Frame AE //
      
0xfc20,
0x1480,
      
0xfc02,
0x3725,
0x45f0,
0x470f,
0x2d00,
0x6001,
      
0xfc00,
0x0200, 

0xff50,
};

//======================================
//저조도 촬영
//======================================

// caMIf_SecTION_Max_ITeMS_100
const unsigned short reg_snapshot_lowlight_list[]  =
{
0xfc01, 
0x0c00, // full YC off //
      
0xfc01,       
0x6F2f, // R gamma //     
0x7052,
0x718f,
0x72e4,
      
0x8723, // G gamma //    
0x8846,
0x8983,
0x8Ad8,
      
0x9F23, // B gamma  //   
0xA046,
0xA183,
0xa2d8,
      
0xfc00, 
0x7301,
0x0348, // AE/AWB Off //
0x7e00,
0x8900,
      
0xfc02,
0x3726, // 25 //
0x45f0,
0x470f,
      
0xfc20,
0x1450, // 50 //
      
0xfc00,
0x0200,
      
0xfc01,
0x3e01,
0x4680,
0x4780,
0x0a03,
0x0ba0,
      
0xfc05,
0x7700,
0x7800,
0x7905,
0x2c20,
0x3020,
      
0xfc02,
0x3c10,
0x028e,   // CIS BPR //
0xfc01,
0x3e00,   // BPR Th1 //
0x3fff,
0x4003,
0x41ff,
0x42ff,
0x4300,
0x4400,

0xff64,
};                     
                       
//======================================
//야간 모드 촬영       
//======================================
                       
// caMIf_SecTION_Max_ITeMS_100
const unsigned short reg_snapshot_nightshot_list[]  =
{                      
0xfc01,       
0x6F40, // R gamma //     
0x706c,
0x71b7,
0x7228,
0x7301,  
      
0x74b8,
      
0x872c, // G gamma //    
0x8854,
0x899f,
0x8A1f,
0x8B01, 
      
0x8Cb8,
      
0x9F45, // B gamma  //   
0xA070,
0xA1bc,
0xa22d,
0xa301, 
      
0xa4bd,  
      
0xfc00,
0x503e,
0x513a,
0x52f5,
0x5320,
0x5440,
0x5554,
0x56f0,
0x5700,
      
0xfc00,
0x0348,
0x7e00,
0x8900,
      
0xfc02,
0x2d00,
0x3738,  // 36 //  
0x470f,
0x45f0,
      
0xfc20,
0x1420, // 50 // 
      
0xfc00,
0x0200,
      
0xfc01,
0x3e00,
0x4668,
0x4768,
0x4915,
0x4814,
0x4b30,
0x4a80,
0x4f30,
0x4e80,
0x0a04,
0x0b00,
      
0xfc05,
0x7760,
0x7860,
0x790f,
0x6dff,
0x6eff,
0x2c30,
0x3030,
      
0xfc02,
0x3c10,
0x028a,   // CIS BPR//
0xfc01,
0x3e00,   // BPR Th1//
0x3fff,
0x4003,
0x41ff,
0x42ff,
0x4300,
0x4400,
      
0xfc00, 
0x0200,


};

 
//======================================
// aWb setting
//======================================

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_wb_auto_list[] =
{
0xfc00,
0x0400,
0x0500,
      
0xfc22,
0x05d0,//d0
0x0400,
0x07ac,//ac
      
0xfc00,
0x3000,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_wb_incandescent_list[] =
{
0xfc00,
0x0400,
0x0500,
      
0xfc22,
0x059d, //a9//
0x0400,
0x07dc, //dc//
      
0xfc00,
0x3002,
};


// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_wb_fluorescent_list[] =
{
0xfc00,
0x0400,
0x0500,
      
0xfc22,
0x05c8,
0x0400,
0x07c5, //c1//
      
0xfc00,
0x3002,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_wb_daylight_list[] =
{
0xfc00,
0x0400,
0x0500,
      
0xfc22,
0x05e2,  //dd//
0x0400,
0x078b,  //8e//
      
0xfc00,
0x3002,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_wb_cloudy_list[] =
{
0xfc00,
0x0400,
0x0500,
      
0xfc22,
0x0511,  //0f//
0x0401,  //00//
0x0778,  //7e//
      
0xfc00,
0x3002,
};

//======================================
// effect setting
// 1. Normal, 2. b/W, 3. Sepia, 4. Green, 5. aqua,
//   6. Negative, 7. embossing, 8. Sketch(Mono)
//======================================

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_none_list[] =
{
0xfc00,
0x2500,

};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_gray_list[] =
{
0xfc00,
0x2508,

};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_sepia_list[] =
{
0xfc00,
0x2501,

};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_green_list[] =
{
0xfc00,
0x2510, 
0xfc01,	
0x05c3,
0xfc0b,
0x0903,
0x0a50,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_aqua_list[] =
{
0xfc00,
0x2502,
0xfc01,
0x05c3,
0xfc0b,
0x0903,
0x0a50
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_negative_list[] =
{
0xfc00,
0x2504,
			
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_sketch_list[] =
{
0xff00,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_effect_emboss_list[] =
{
0xff00,
};

//======================================
// Metering setting
//======================================

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_meter_frame_list[] =
{
0xff00,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_meter_center_list[] =
{
0xff00,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_meter_spot_list[] =
{
0xff00,
};


//======================================
// brightness setting  
//======================================

// -4
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_0_list[] =
{
0xfc00,
0x2901,
0x2a75,

};

// -3
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_1_list[] =
{
0xfc00,
0x2902,
0x2a17,

};

// -2
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_2_list[] =
{
0xfc00,
0x2902,
0x2ab9,

};

// -1
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_3_list[] =
{
0xfc00,
0x2903,
0x2a5b,

};

// 0
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_4_list[] =
{
0xfc00,
0x2904,
0x2a00,

};

// +1
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_5_list[] =
{
0xfc00,
0x2904,
0x2a9b,

};

// +2
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_6_list[] =
{
0xfc00,
0x2905,
0x2a36,

};

// +3
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_7_list[] =
{
0xfc00,
0x2905,
0x2ad1,

};

// +4
// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_brightness_8_list[] =
{
0xfc00,
0x2906,
0x2a6c, //  a0

};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_nightshot_on_list[] =
{
0xfc02,
0x2d00,
0x371e, /*Global gain*/

0x44db,
0x45f0,
0x470f,
0x4a30,
0x4b33,
0x4d02,
0x4f2a,
0x551f,
0x5b0f,
0x6209,
0x6000,
0x6311,

0xfc00,
0x7331, /*ae*/
0x786a, /*agc max*/

0xfc20,
0x1470,
};

// caMIf_SecTION_Max_ITeMS_50
const unsigned short reg_nightshot_off_list[] =
{      
0xfc02,
0x2d58,
0x3718, /*G gain*/

0x44db,
0x45f0,
0x472f,
0x4a00,
0x4b11,
0x4d02,
0x4faa,
0x551f,
0x5b0f,
0x620d,
0x6001,
0x6322,

0xfc00,
0x7311, /*ae*/
0x786a, /*agc max*/

0xfc20,
0x1480,
};     

const unsigned short reg_shade_dnp_list[] = 
{
// shade_dnp //
0xfc09,
0x3500,   
0x36fd,   
0x3701,   
0x380c,   
0x3901,   
0x3a3f,   
0x3b01,   
0x3c82,   
0x3d01,   
0x3ea3,   
0x3f01,   
0x40ba,   
0x4101,   
0x42df,   
0x4301,   
0x44f9,
      
0x5618,
0x5825,
0x0002,  // shading on //
//0xfc00,
//0x2f02,
};

const unsigned short reg_shade_etc_list[] = 
{
// shade_etc //              
0xfc09,
0x3501,//01   
0x3604,//02  
0x3701,//01   
0x3814,//12   
0x3901,//01   
0x3a41,//3f   
0x3b01,//01   
0x3c79,//7d   
0x3d01,//01   
0x3e96,//9a  
0x3f01,//01   
0x40b3,//b8   
0x4101,//01   
0x42d8,//dd   
0x4301,//01   
0x44f2,//f7
         
0x5614,//14
0x5821,//21
0x0002,//02  // shading on //

//0xfc00,
//0x2f01,
};

const unsigned short reg_AgcValue_list[] =
{
0x0002, //normal light condition
0x0052, //mid_low light condition
0x006a, //low light condition
0x0038, //shade_min
0x007f, //shade_max
};


///////////////////////////////////////////////////////
const unsigned short reg_DTP_list[] =
{
0xfc00,
0x0200,

//liner set
////////////////////////////////
// AE Window Weight linear(EVT1)0929   
0xfc20, // upper window weight zero
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
0x7811,
0x7911,
0x7a11,
0x7b11,
0x7c11,
0x7d11,
0x7e11,
0x7f11,

// AE window Weight setting End
//color matrix linear //
0xfc01,    
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

//gamma linear //

0xfc01,                
0x6f08, // R         
0x7010,                
0x7120,                
0x7240,                
0x7300,                
0x7480,                
0x75c0,                
0x7600,                
0x7740,                
0x7805,                
0x7980,                
0x7ac0,                
0x7b00,                
0x7c40,                
0x7d5a,                
0x7e80,                
0x7fc0,                
0x8000,                
0x8140,                
0x82af,                
0x8380,                
0x84c0,                
0x85ff,                
0x86fc,                
0x8708,   //G          
0x8810,                
0x8920,                
0x8a40,                
0x8b00,                
0x8c80,                
0x8dc0,                
0x8e00,                
0x8f40,                
0x9005,                
0x9180,                
0x92c0,                
0x9300,                
0x9440,                
0x955a,                
0x9680,                
0x97c0,                
0x9800,                
0x9940,                
0x9aaf,                
0x9b80,                
0x9cc0,                
0x9dff,                
0x9efc,                
0x9f08,   //B          
0xa010,                
0xa120,                
0xa240,                
0xa300,                
0xa480,                
0xa5c0,                
0xa600,                
0xa740,                
0xa805,                
0xa980,                
0xaac0,                
0xab00,                
0xac40,                
0xad5a,                
0xae80,                
0xafc0,                
0xb000,                
0xb140,                
0xb2af,                
0xb380,                
0xb4c0,                
0xb5ff,                
0xb6fc,                

//hue gain linear //
0xfc00,                                
0x4840,     
0x4940,     
0x4a00,     
0x4b00,     
0x4c40,     
0x4d40,     
0x4e00,     
0x4f00,     
0x5040,     
0x5140,     
0x5200,     
0x5300,     
0x5440,     
0x5540,     
0x5600,     
0x5700,     
0x5840,     
0x5940,     
0x5a00,     
0x5b00,     
0x5c40,     
0x5d40,     
0x5e00,     
0x5f00,     


//etc linear //
0xfc00,
0x6200,

0xfc00,                                             
0x7bff,                                  
                                        
0x2904, // brightness                   
0x2a00,                                   
0x2b04, // color level                  
0x2c00,                                   
                                        
0x7900, //R offset                      
0x7a00, // B offset                     
                                        
0xfc07,                                   
0x1100, // G offset                                                          
                                        
0xfc05,                                   
0x4e00,                                   
0x4f00,                                   
0x5044,                                   
0x5100,                                   
0x5200,                                   
0x5300,                                  
0x5400,                                   
0x5500,                                   
0x5644,                                   
0x5700,                                   
0x5800,                                   
0x5900,                                   
                                        
0x6780, // AWB R White Gain             
0x6840, // AWB G white Gain             
0x6980, // AWB B white gain 

0xfc00,
0x7b00,

0xfc09,
0x0000,

0xfc00,
0x0348,

0xfc00,
0x0201,	// Mode change

0xfc01,
0x0202,	//StealthQ : Cr_Y_Cb_Y //ORG: 0203 - Cb_Y_Cr_Y ,//YCorder

//color bar
0x3d11,
};

