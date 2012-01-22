#!/usr/bin/perl -Xw

#****************************************************************************
#**
#**         smem_log.pl
#**
#** 
#** Copyright (c) 2005 - 2010 by QUALCOMM, Incorporated.  
#** All Rights Reserved.
#****************************************************************************
#**
#**                        EDIT HISTORY FOR MODULE
#**
#**  $Header: //source/qcom/qct/core/pkg/2H09/halcyon_modem/main/latest/AMSS/products/7x30/tools/debug/smem_log.pl#9 $
#**
#** when       who     what, where, why
#** --------   ---     ------------------------------------------------------
#** 01/12/10   ap      Added support for router clients waiting on server reg.
#** 09/30/09   ih      Added profiling support
#** 09/09/09   haa     Added CLKRGM Power logging
#** 07/30/09   ajf     Updated DEM state names to reflect sleep prepare state,
#**                    and support for extended sleep messages.
#** 06/24/09   aep     Support extra smem logging in oncrpc.
#** 05/07/09   sa      Added parsing of router ping message
#** 03/06/09   tl      Suppress log output if log was not initialized
#** 01/16/09   taw     Added support for the DEM INIT event.
#** 01/16/08   ajf     Refactored some of the DEM messages to be more n-way generic.
#** 01/09/09   tl      Added support for power circular log
#** 01/05/09   hwu     Revert the SMSM bits cleanup changes to be backward compatible.
#** 11/26/08   hwu     SMSM bits cleanup.
#** 11/17/08   ajf     Updated the DEM message IDs and added additional decode
#**                    support for new N-way DEM messages.
#** 11/11/08   ih      Added new SMSM bits
#** 10/15/08   ajf     Support for new DEM time sync messages.
#** 08/20/08   taw     Added support for ext. wakeup reasons to DEM logging.
#** 03/20/08   sa      Added support for rpc router messages.
#** 03/11/08   taw     Fix printing of modem non-voters.
#** 01/28/08   taw     Fix the way the sleep voter names are parsed.
#** 01/03/08   ptm     Fix problem with circular log show log entries multiple
#**                    times.
#** 10/05/07   taw     When logging sleep voters, don't reverse the names.
#** 09/26/07   taw     Added support for a new DEM event; also, updated the
#**                    list of wakeup interrupts.
#** 09/14/07   hwu     Added to parse QDSP log. Also update SMD stream states.
#** 08/26/07   taw     Check for the existence of static log files before
#**                    trying to parse them.
#** 08/24/07   taw     Changed the name of the ADSP common interrupt; removed
#**                    sleep_shared_int_names (not used).
#** 08/16/07   ebb     Added support to parse static log files.
#** 08/13/07   ebb     Added support to parse the static sleep voters.
#** 07/11/07   ebb     Added support for SLEEP log events.
#** 07/04/07   hwu     Updates the smsm state machine states.
#** 04/04/07   ptm     Added support for SMEM DM and interrupt events.
#** 03/01/07   taw     Added support for new DEM event, "SETUP APPS SUSPEND".
#** 12/04/06   hwu     Added support for newly added PROGs.
#** 11/21/06   hwu     Added to parse DCVS log messages.
#** 10/03/06   ddh     Corrected info in DEM debug
#** 08/08/06   cab     Added UMTS support
#** 06/27/06   ptm     Added RPC ASYNC call and error fatal task events.
#** 05/27/06   ptm     Added SMSM state printing and new ONCRPC events.
#** 05/24/06   ddh     Added new DEM event for when the apps executes SWFI
#** 05/16/06   ptm     Added ERR support
#** 05/10/06   taw     Added new DEM events, and support for printing 
#**                    DEM wakeup interrupts.
#** 04/10/06   ptm     Changed file parser to handle trace32 wp.v.v files.
#** 04/06/06   ddh     Added DEM support
#** 07/19/05   ptm     
#** 03/23/05   ~SN     Initial Version.
#****************************************************************************

use FileHandle;

# This has to match with NUM_LOG_ENTRIES in drivers/smem/smem_logi.h
my $MAX_LENGTH = 2000;
my $MAX_STATIC_LENGTH = 150;

my $LSB_MASK      = 0x0000ffff;
my $CONTINUE_MASK = 0x30000000;
my $BASE_MASK     = 0x0fff0000;

my $SMEM_LOG_DEBUG_EVENT_BASE         = 0x00000000;
my $SMEM_LOG_ONCRPC_EVENT_BASE        = 0x00010000;
my $SMEM_LOG_SMEM_EVENT_BASE          = 0x00020000;
my $SMEM_LOG_TMC_EVENT_BASE           = 0x00030000;
my $SMEM_LOG_TIMETICK_EVENT_BASE      = 0x00040000;
my $SMEM_DEM_EVENT_BASE               = 0x00050000;
my $SMEM_ERR_EVENT_BASE               = 0x00060000;
my $SMEM_LOG_DCVS_EVENT_BASE          = 0x00070000;
my $SMEM_LOG_SLEEP_EVENT_BASE         = 0x00080000;
my $SMEM_LOG_RPC_ROUTER_EVENT_BASE    = 0x00090000;
my $SMEM_LOG_CLKREGIM_EVENT_BASE      = 0x000A0000;

# UMTS blocks out a subset of DEM event space for UMTS use
my $DEM_UMTS_BASE = 0x00008000;

my $LINE_HEADER;

main();

exit;

sub parse_args {
  my $arg = shift @_;

  my $help          = FALSE;
  my $relative_time = FALSE;
  my $ticks         = FALSE;

  while( defined $arg ) {
    if( $arg eq "--help" ) {
      $help = TRUE;
      $arg = shift @_;
      next;
    }

    if( $arg eq "-r" ) {
      $relative_time = TRUE;
      $arg = shift @_;
      next;
    }

    if( $arg eq "-t" ) {
      $ticks = TRUE;
      $arg = shift @_;
      next;
    }
  }

  if( $help eq TRUE ) {
    print STDERR "usage: smem_log.pl [-r]\n";
    print STDERR "       -r : print relative time instead of absolute\n";
    print STDERR "       -t : print time in ticks instead of seconds\n";
    exit
  }

  return $relative_time, $ticks;
} # parse_args

###########################################################################
#        Trace32 file read and parse routines
###########################################################################

sub parse_log_file {
  my( $file_name ) = @_;
  my $fd = new FileHandle;
  my @recs;

  open $fd, "<$file_name" or die "Unable to open \"$file_name\"";

 while( <$fd> ) {
   if( m/smem_log_events = (0x[0-9a-f]+)\b/i ) {
     # If this smem log is not initialized, this log pointer will be NULL.
     # In this case, skip the log.
     if( $1 eq '0x0' ) {
       return [];
     }
   }
   if( m/.*?= (\w+).*?= (\w+).*?= (\w+).*?= (\w+).*?= (\w+)/ ) {
     push @recs, [hex $1, hex $2, hex $3, hex $4, hex $5 ];
   }
 }
  return \@recs;
} # parse_log_file

sub parse_index_file {
  my( $file_name ) = @_;
  my $fd = new FileHandle;
  my $idx = 0;

  open $fd, "<$file_name" or die "Unable to open \"$file_name\"";

  # Skip first line - header junk
  <$fd>;

  $_ = <$fd>;
  if( m/.*?= (\w+)/ ) {
    $idx = hex $1;
  }

  return $idx;
} # parse_index_file

# Parse the dynamic sleep
sub parse_voter_file {

  my $filename = $_[0];
  open(LOG_FILE, "< $filename");

  # Change this if either of these change in C code.
  my $MAX_NUM_SLEEP_CLIENTS = 64;
  my $SLEEPLOG_CLIENT_LIST_MAX_STR = 9;

  my $name = "";
  my $count = 0;
  my @mod_voters;
  my @app_voters;

  # Ignore first two lines
  <LOG_FILE>;
  <LOG_FILE>;

  my $byte_cnt = 0;
  while( <LOG_FILE> )
  {
    chomp( $_ );
    if(/\s+(0x.+),/)
    {

      my $word = hex( $1 );
      $word = swap_bytes( $word ); 

      #Loop through each byte                 
      for($i=0; $i<4; $i++)                   
      {                  
        $byte_cnt++;

        my $c = ($word & (0xFF << 24) ) >> 24;  

        #If we hit a null char, store word   
        if( $c == 0 && $name ne "")          
        {                  
           # Check if we are in the modem address space
          if( $byte_cnt < 
              ( $MAX_NUM_SLEEP_CLIENTS * $SLEEPLOG_CLIENT_LIST_MAX_STR ) )
          {
             push( @mod_voters, $name );
          }
          else
          {
             push( @app_voters, $name );
          }
          $name = "";                       
        }                                    
        #else append onto string             
        elsif($c != 0)                       
        {                                    
          $name .= chr( $c );               
        }                                    

        #Shift in next byte                  
        $word <<= 8;                            
      }                                       

      # Increment entry count
      $count++;
    }
  }
  close(LOG_FILE);

  return ( \@mod_voters, \@app_voters );

 # Swaps bytes
  sub swap_bytes {
    my $word = $_[0];
    my $swapped = 0;

    $swapped |= ( ($word & (0xFF << 0))   << 24);
    $swapped |= ( ($word & (0xFF << 8))   << 8);
    $swapped |= ( ($word & (0xFF << 16))  >> 8);
    $swapped |= ( ($word & (0xFF << 24))  >> 24);

    return $swapped;
  }

}



###########################################################################
#        ONCRPC Definitions and print routines
###########################################################################

BEGIN {        # Use begin block so that these variables are defined at runtime
  $RPC_PRINT_TABLE = [ "GOTO SMD WAIT",
                       "GOTO RPC WAIT",
                       "GOTO RPC BOTH WAIT",
                       "GOTO RPC INIT",
                       "GOTO RUNNING",
                       "APIS INITED",
                       "AMSS RESET - GOTO SMD WAIT",
                       "SMD RESET - GOTO SMD WAIT",
                       "ONCRPC RESET - GOTO SMD WAIT",
                       EVENT_CB,
                       STD_CALL,
                       STD_REPLY,
                       STD_CALL_ASYNC,
                       ERR_NO_PROC,
                       ERR_DECODE,
                       ERR_SYSTEM,
                       ERR_AUTH,
                       ERR_NO_PROG,
                       ERR_PROG_LOCK,
                       ERR_PROG_VERS,
                       ERR_VERS_MISMATCH,
                       CALL_START,
                       DISPATCH_PROXY,
                       HANDLE_CALL,
                       MSG_DONE ];

  $RPC_PROG_NAMES = { 0x30000000 => "CM",
                      0x30000001 => "DB",
                      0x30000002 => "SND",
                      0x30000003 => "WMS",
                      0x30000004 => "PDSM",
                      0x30000005 => "MISC_MODEM_APIS",
                      0x30000006 => "MISC_APPS_APIS",
                      0x30000007 => "JOYST",
                      0x30000008 => "VJOY",
                      0x30000009 => "JOYSTC",
                      0x3000000a => "ADSPRTOSATOM",
                      0x3000000b => "ADSPRTOSMTOA",
                      0x3000000c => "I2C",
                      0x3000000d => "TIME_REMOTE",
                      0x3000000e => "NV",
                      0x3000000f => "CLKRGM_SEC",
                      0x30000010 => "RDEVMAP",
                      0x30000011 => "FS_RAPI",
                      0x30000012 => "PBMLIB",
                      0x30000013 => "AUDMGR",
                      0x30000014 => "MVS",
                      0x30000015 => "DOG_KEEPALIVE",
                      0x30000016 => "GSDI_EXP",
                      0x30000017 => "AUTH",
                      0x30000018 => "NVRUIMI",
                      0x30000019 => "MMGSDILIB",
                      0x3000001a => "CHARGER",
                      0x3000001b => "UIM",
                      0x3000001C => "ONCRPCTEST",
                      0x3000001d => "PDSM_ATL",
                      0x3000001e => "FS_XMOUNT",
                      0x3000001f => "SECUTIL ",
                      0x30000020 => "MCCMEID",
                      0x30000021 => "PM_STROBE_FLASH",
                      0x30000022 => "DS707_EXTIF",
                      0x30000023 => "SMD BRIDGE_MODEM",
                      0x30000024 => "SMD PORT_MGR",
                      0x30000025 => "BUS_PERF",
                      0x30000026 => "BUS_MON",
                      0x30000027 => "MC",
                      0x30000028 => "MCCAP",
                      0x30000029 => "MCCDMA",
                      0x3000002a => "MCCDS",
                      0x3000002b => "MCCSCH",
                      0x3000002c => "MCCSRID",
                      0x3000002d => "SNM",
                      0x3000002e => "MCCSYOBJ",
                      0x3000002f => "DS707_APIS",
                      0x30000030 => "DS_MP_SHIM_APPS_ASYNC",
                      0x30000031 => "DSRLP_APIS",
                      0x30000032 => "RLP_APIS",
                      0x30000033 => "DS_MP_SHIM_MODEM",
                      0x30000034 => "DSHDR_APIS",
                      0x30000035 => "DSHDR_MDM_APIS",
                      0x30000036 => "DS_MP_SHIM_APPS",
                      0x30000037 => "HDRMC_APIS",
                      0x30000038 => "SMD_BRIDGE_MTOA",
                      0x30000039 => "SMD_BRIDGE_ATOM",
                      0x3000003a => "DPMAPP_OTG",
                      0x3000003b => "DIAG",
                      0x3000003c => "GSTK_EXP",
                      0x3000003d => "DSBC_MDM_APIS",
                      0x3000003e => "HDRMRLP_MDM_APIS",
                      0x3000003f => "HDRMRLP_APPS_APIS",
                      0x30000040 => "HDRMC_MRLP_APIS",
                      0x30000041 => "PDCOMM_APP_API",
                      0x30000042 => "DSAT_APIS",
                      0x30000043 => "MISC_RF_APIS",
                      0x30000044 => "CMIPAPP",
                      0x30000045 => "DSMP_UMTS_MODEM_APIS",
                      0x30000046 => "DSMP_UMTS_APPS_APIS",
                      0x30000047 => "DSUCSDMPSHIM",
                      0x30000048 => "TIME_REMOTE_ATOM",
                      0x3000004a => "SD",
                      0x3000004b => "MMOC",
                      0x3000004c => "WLAN_ADP_FTM",
                      0x3000004d => "WLAN_CP_CM",
                      0x3000004e => "FTM_WLAN",
                      0x3000004f => "SDCC_CPRM",
                      0x30000050 => "CPRMINTERFACE",
                      0x30000051 => "DATA_ON_MODEM_MTOA_APIS",
                      0x30000052 => "DATA_ON_APPS_ATOM_APIS",
                      0x30000053 => "MISC_MODEM_APIS_NONWINMOB",
                      0x31000000 => "CM CB",
                      0x31000001 => "DB CB",
                      0x31000002 => "SND CB",
                      0x31000003 => "WMS CB",
                      0x31000004 => "PDSM CB",
                      0x31000005 => "MISC_MODEM_APIS CB",
                      0x31000006 => "MISC_APPS_APIS CB",
                      0x31000007 => "JOYST CB",
                      0x31000008 => "VJOY CB",
                      0x31000009 => "JOYSTC CB",
                      0x3100000a => "ADSPRTOSATOM CB",
                      0x3100000b => "ADSPRTOSMTOA CB",
                      0x3100000c => "I2C CB",
                      0x3100000d => "TIME_REMOTE CB",
                      0x3100000e => "NV CB",
                      0x3100000f => "CLKRGM_SEC CB",
                      0x31000010 => "RDEVMAP CB",
                      0x31000011 => "FS_RAPI CB",
                      0x31000012 => "PBMLIB CB",
                      0x31000013 => "AUDMGR CB",
                      0x31000014 => "MVS CB",
                      0x31000015 => "DOG_KEEPALIVE CB",
                      0x31000016 => "GSDI_EXP CB",
                      0x31000017 => "AUTH CB",
                      0x31000018 => "NVRUIMI CB",
                      0x31000019 => "MMGSDILIB CB",
                      0x3100001a => "CHARGER CB",
                      0x3100001b => "UIM CB", 
                      0x3100001C => "ONCRPCTEST CB",
                      0x3100001d => "PDSM_ATL CB",
                      0x3100001e => "FS_XMOUNT CB",
                      0x3100001f => "SECUTIL CB",
                      0x31000020 => "MCCMEID",
                      0x31000021 => "PM_STROBE_FLASH CB",
                      0x31000022 => "DS707_EXTIF CB",
                      0x31000023 => "SMD BRIDGE_MODEM CB",
                      0x31000024 => "SMD PORT_MGR CB",
                      0x31000025 => "BUS_PERF CB",
                      0x31000026 => "BUS_MON CB",
                      0x31000027 => "MC CB",
                      0x31000028 => "MCCAP CB",
                      0x31000029 => "MCCDMA CB",
                      0x3100002a => "MCCDS CB",
                      0x3100002b => "MCCSCH CB",
                      0x3100002c => "MCCSRID CB",
                      0x3100002d => "SNM CB",
                      0x3100002e => "MCCSYOBJ CB",
                      0x3100002f => "DS707_APIS CB",
                      0x31000030 => "DS_MP_SHIM_APPS_ASYNC CB",
                      0x31000031 => "DSRLP_APIS CB",
                      0x31000032 => "RLP_APIS CB",
                      0x31000033 => "DS_MP_SHIM_MODEM CB",
                      0x31000034 => "DSHDR_APIS CB",
                      0x31000035 => "DSHDR_MDM_APIS CB",
                      0x31000036 => "DS_MP_SHIM_APPS CB",
                      0x31000037 => "HDRMC_APIS CB",
                      0x31000038 => "SMD_BRIDGE_MTOA CB",
                      0x31000039 => "SMD_BRIDGE_ATOM CB",
                      0x3100003a => "DPMAPP_OTG CB",
                      0x3100003b => "DIAG CB",
                      0x3100003c => "GSTK_EXP CB",
                      0x3100003d => "DSBC_MDM_APIS CB",
                      0x3100003e => "HDRMRLP_MDM_APIS CB",
                      0x3100003f => "HDRMRLP_APPS_APIS CB",
                      0x31000040 => "HDRMC_MRLP_APIS CB",
                      0x31000041 => "PDCOMM_APP_API CB",
                      0x31000042 => "DSAT_APIS CB",
                      0x31000043 => "MISC_RF_APIS CB",
                      0x31000044 => "CMIPAPP CB",
                      0x31000045 => "DSMP_UMTS_MODEM_APIS CB",
                      0x31000046 => "DSMP_UMTS_APPS_APIS CB",
                      0x31000047 => "DSUCSDMPSHIM CB",
                      0x31000048 => "TIME_REMOTE_ATOM CB",
                      0x3100004a => "SD CB",
                      0x3100004b => "MMOC CB",
                      0x3100004c => "WLAN_ADP_FTM CB",
                      0x3100004d => "WLAN_CP_CM CB",
                      0x3100004e => "FTM_WLAN CB",
                      0x3100004f => "SDCC_CPRM CB",
                      0x31000050 => "CPRMINTERFACE CB",
                      0x31000051 => "DATA_ON_MODEM_MTOA_APIS CB",
                      0x31000052 => "DATA_ON_APPS_ATOM_APIS CB",
                      0x31000053 => "MISC_APIS_NONWINMOB CB" };
} # BEGIN

sub oncrpc_print {
  my( $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$RPC_PRINT_TABLE[$event];

  if( $cntrl eq ERR_NO_PROC ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR PROC NOT SUPPORTED   xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR PROC NOT SUPPORTED  xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq ERR_DECODE ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR ARGS DECODE FAILED   xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR ARGS DECODE FAILED  xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq ERR_SYSTEM ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR SYSTEM FAULT  xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR SYSTEM FAULT  xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq ERR_AUTH ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR AUTHENTICATION FAILED  xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR AUTHENTICATION FAILED  xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq ERR_NO_PROG ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR PROG NOT EXPORTED  xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR PROG NOT EXPORTED  xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq ERR_PROG_LOCK ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR PROG LOCKED  xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR PROG LOCKED  xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq ERR_PROG_VERS ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR PROG VERS NOT SUPPORED  xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR PROG VERS NOT SUPPORTED  xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq ERR_VERS_MISMATCH ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};
    if( defined $prog ) {
      printf( "ONCRPC: ERROR RPC VERS NOT SUPPORTED   xid = %08x    proc = %3d    prog = $prog",
              $d1, $d3 );
    }
    else {
      printf( "ONCRPC: ERROR RPC VERS NOT SUPPORTED   xid = %08x    proc = %3d    prog = %08x",
              $d1, $d3, $d2 );
    }
  }
  elsif( $cntrl eq EVENT_CB ) {
    printf( "ONCRPC: EVENT_CB  %s\n",
            $d1 eq 1 ? "LOCAL" : "REMOTE" );
    printf( "%sONCRPC: prev = %s\n", $LINE_HEADER, smsm_state_to_string($d2) );
    printf( "%sONCRPC: curr = %s\n", $LINE_HEADER, smsm_state_to_string($d3) );
  }
  elsif( $cntrl eq STD_CALL or $cntrl eq NBS_CALL ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};

    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First CALL record
      if( defined $prog ) {
        printf( "ONCRPC: CALL    xid = %08x    proc = %3d    prog = $prog",
                $d1, $d3 );
      }
      else {
        printf( "ONCRPC: CALL    xid = %08x    proc = %3d    prog = %08x",
                $d1, $d3, $d2 );
      }
    }
    else {
      # Second CALL record
      my $name = pack( "I3", $d1, $d2, $d3);
      $name =~ s/\0.*//;
      print "    task = $name\n";
    }
  }
  elsif( $cntrl eq STD_REPLY or $cntrl eq NBS_REPLY ) {
    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First REPLY record
      printf( "ONCRPC: REPLY   xid = %08x", $d1 );
    }
    else {
      # Second REPLY record
      my $name = pack( "I3", $d1, $d2, $d3);
      $name =~ s/\0.*//;
      print "    task = $name\n";
    }
  }
  elsif( $cntrl eq STD_CALL_ASYNC or $cntrl eq NBS_CALL_ASYNC ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};

    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First CALL record
      if( defined $prog ) {
        printf( "ONCRPC: ASYNC   xid = %08x    proc = %3d    prog = $prog",
                $d1, $d3 );
      }
      else {
        printf( "ONCRPC: ASYNC   xid = %08x    proc = %3d    prog = %08x",
                $d1, $d3, $d2 );
      }
    }
    else {
      # Second CALL record
      my $name = pack( "I3", $d1, $d2, $d3);
      $name =~ s/\0.*//;
      print "    task = $name\n";
    }
  }
  elsif( $cntrl eq CALL_START or $cntrl eq NBS_START ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};

    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First CALL record
      if( defined $prog ) {
        printf( "ONCRPC: START   xid = %08x    proc = %3d    prog = $prog",
                $d1, $d3 );
      }
      else {
        printf( "ONCRPC: START   xid = %08x    proc = %3d    prog = %08x",
                $d1, $d3, $d2 );
      }
    }
    else {
      # Second CALL record
      my $name = pack( "I3", $d1, $d2, $d3);
      $name =~ s/\0.*//;
      print "    task = $name\n";
    }
  }
  elsif( $cntrl eq DISPATCH_PROXY or $cntrl eq NBS_DISPATCH ) {
    printf( "ONCRPC: DISPATCH  xid = %08x    func = %08x\n", $d1, $d2 );
  }
  elsif( $cntrl eq HANDLE_CALL or $cntrl eq NBS_HANDLE ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};

    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First CALL record
      if( defined $prog ) {
        printf( "ONCRPC: HANDLE  xid = %08x    proc = %3d    prog = $prog",
                $d1, $d3 );
      }
      else {
        printf( "ONCRPC: HANDLE  xid = %08x    proc = %3d    prog = %08x",
                $d1, $d3, $d2 );
      }
    }
    else {
      # Second CALL record
      my $name = pack( "I3", $d1, $d2, $d3);
      $name =~ s/\0.*//;
      print "    task = $name\n";
    }
  }
  elsif( $cntrl eq MSG_DONE or $cntrl eq NBS_DONE ) {
    my $prog  = $$RPC_PROG_NAMES{$d2};

    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First CALL record
      if( defined $prog ) {
        printf( "ONCRPC: DONE    xid = %08x    proc = %3d    prog = $prog",
                $d1, $d3 );
      }
      else {
        printf( "ONCRPC: DONE    xid = %08x    proc = %3d    prog = %08x",
                $d1, $d3, $d2 );
      }
    }
    else {
      # Second CALL record
      my $name = pack( "I3", $d1, $d2, $d3);
      $name =~ s/\0.*//;
      print "    task = $name\n";
    }
  }
  else {
    generic_print( $RPC_PRINT_TABLE, "ONCRPC", @_ );
  }
} # oncrpc_print

sub smsm_state_to_string {
  my( $state ) = @_;
  my $string = "";

  $string .= ($state & 0x80000000) ? "UN " : "   ";
  $string .= ($state & 0x70000000) ? "ERR " : "    ";
  $string .= ($state & 0x08000000) ? "MWT " : "    ";
  $string .= ($state & 0x04000000) ? "MCNT " : "    ";
  $string .= ($state & 0x02000000) ? "MBRK " : "    ";
  $string .= ($state & 0x01000000) ? "RNQT " : "    ";
  $string .= ($state & 0x00800000) ? "SMLP " : "    ";
  $string .= ($state & 0x00400000) ? "ADWN " : "    ";
  $string .= ($state & 0x00200000) ? "PWRS " : "    ";
  $string .= ($state & 0x00100000) ? "DWLD " : "    ";
  $string .= ($state & 0x00080000) ? "SRBT " : "    ";
  $string .= ($state & 0x00040000) ? "SDWN " : "    ";
  $string .= ($state & 0x00020000) ? "ARBT " : "    ";
  $string .= ($state & 0x00010000) ? "REL " : "    ";
  $string .= ($state & 0x00008000) ? "SLE " : "    ";
  $string .= ($state & 0x00004000) ? "SLP " : "    ";
  $string .= ($state & 0x00002000) ? "WFPI " : "     ";
  $string .= ($state & 0x00001000) ? "EEX " : "    ";
  $string .= ($state & 0x00000800) ? "TIN " : "    ";
  $string .= ($state & 0x00000400) ? "TWT " : "    ";
  $string .= ($state & 0x00000200) ? "PWRC " : "     ";
  $string .= ($state & 0x00000100) ? "RUN " : "    ";
  $string .= ($state & 0x00000080) ? "SA " : "   ";
  $string .= ($state & 0x00000040) ? "RES " : "    ";
  $string .= ($state & 0x00000020) ? "RIN " : "    ";
  $string .= ($state & 0x00000010) ? "RWT " : "    ";
  $string .= ($state & 0x00000008) ? "SIN " : "    ";
  $string .= ($state & 0x00000004) ? "SWT " : "    ";
  $string .= ($state & 0x00000002) ? "OE " : "   ";
  $string .= ($state & 0x00000001) ? "I" : " ";

  return $string;
} # smsm_state_to_string

###########################################################################
#        SMEM Definitions and print routines
###########################################################################

BEGIN {        # Use begin block so that these variables are defined at runtime
  @smem_smsm_state_names = ( "IGNORE ", "WAIT   ", "INIT   ", "RUNNING" );
  @smem_stream_state_names = ( "CLOSED", "OPENING", "OPENED",
                               "FLUSHING", "CLOSING", "RESET",
                               "RESET_OPENING" );
  @smem_stream_event_names = ( "CLOSE", "OPEN", "REMOTE OPEN",
                               "REMOTE CLOSE", "FLUSH",
                               "FLUSH COMPLETE", "REMOTE_REOPEN",
                               "REMOTE_RESET");

  $SMEM_PRINT_TABLE = [ EVENT_CB,
                        "START",
                        "INIT",
                        "RUNNING",
                        "STOP",
                        "RESTART",
                        EVENT_SS,
                        EVENT_READ,
                        EVENT_WRITE,
                        EVENT_SIGS1,
                        EVENT_SIGS2,
                        "WRITE DM",
                        "READ DM",
                        "SKIP DM",
                        "STOP DM",
                        "ISR",
                        "TASK" ];
} # BEGIN

sub smem_print {
  my( $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$SMEM_PRINT_TABLE[$event];

  if( $cntrl eq EVENT_CB ) {
    printf( "SMEM:   EVENT_CB  %s\n",
            $d1 eq 1 ? "LOCAL" : "REMOTE" );
    printf( "%sSMEM:   prev = %s\n", $LINE_HEADER, smsm_state_to_string($d2) );
    printf( "%sSMEM:   curr = %s\n", $LINE_HEADER, smsm_state_to_string($d3) );
  }
  elsif( $cntrl eq EVENT_SS ) {
    printf( "SMEM: EVENT SS  port %d  state = %s  event = %s\n",
            $d1,
            $smem_stream_state_names[$d2],
            $smem_stream_event_names[$d3] );
  }
  elsif( $cntrl eq EVENT_READ ) {
    if( $d1 eq 0 ) {
      printf( "SMEM: EVENT READ enter port %d\n", $d2 );
    }
    else {
      printf( "SMEM: EVENT READ exit port %d (%d)\n", $d2, $d1 );
    }
  }
  elsif( $cntrl eq EVENT_WRITE ) {
    if( $d1 eq 0 ) {
      printf( "SMEM: EVENT WRITE enter port %d\n", $d2 );
    }
    else {
      printf( "SMEM: EVENT WRITE exit port %d (%d)\n", $d2, $d1 );
    }
  }
  elsif( $cntrl eq EVENT_SIGS1 ) {
    printf( "SMEM: EVENT SIGS  port %d received %08x converted to %08x\n",
            $d1, $d2, $d3 );
  }
  elsif( $cntrl eq EVENT_SIGS2 ) {
    printf( "SMEM: EVENT_SIGS  port %d  old = %08x  new = %08x\n",
            $d1, $d2, $d3 );
  }
  else {
    generic_print( $SMEM_PRINT_TABLE, "SMEM", @_ );
  }
} # smem_print

###########################################################################
#        DEM Definitions and print routines
###########################################################################
BEGIN {        # Use begin block so that these variables are defined at runtime

  @dem_shared_int_names = ( "MDDI_EXT", "MDDI_PRI", "MDDI_CLIENT", 
                            "USB_OTG", "I2CC", "SDC1_0", 
                            "SDC1_1", "SDC2_0", "SDC2_1",
                            "ADSP_A9A11", "UART1", "UART2", "UART3",
                            "DP_RX_DATA", "DP_RX_DATA2", "DP_RX_DATA3",
                            "DM_UART", "DM_DP_RX_DATA", "KEYSENSE", "HSSD",
                            "NAND_WR_ER_DONE", "NAND_OP_DONE", "TCHSCRN1",
                            "TCHSCRN2", "TCHSCRN_SSBI", "USB_HS", "UART2_DM_RX",
                            "UART2_DM", "SDC4_1", "SDC4_0", "SDC3_1", "SDC3_0"
                          );

  # These need to match the list of wake up reasons in dem.h.
  @dem_wakeup_reason_names = ( "SMD", "INT", "GPIO", "TIMER", "ALARM",
                               "RESET", "OTHER", "REMOTE"
                             );

  # These need to match the information in smsm.h.  Also, the logic for
  # decoding the SMSM_ISR message has some use of this array hardcoded.
  @dem_cpu_names = ( "APPS", "MODEM", "QDSP6" );
  @smsm_entry_names = ( "APPS SMSM STATE", "MODEM SMSM STATE", "Q6 SMSM STATE",
                        "APPS DEM STATE", "MODEM DEM STATE", "Q6 DEM STATE",
                        "POWER MASTER STATE", "TIME MASTER STATE" );
  @smsm_state_bit_names = ( "SMSM_INIT", "OSENTERED", "SMDWAIT", "SMDINIT",
                            "RPCWAIT", "RPCINIT", "RESET", "unused", 
                            "unused", "unused", "unused", "unused", 
                            "unused", "unused", "unused", "unused", 
                            "OEMSBL_RELEASE", "APPS_REBOOT","SYS_POWERDOWN", "SYS_REBOOT", 
                            "SYS_DOWNLOAD","unused", "APPS_SHUTDOWN", "SMD_LOOPBACK",
                            "unused", "MODM_WAIT","MODM_BREAK", "MODM_CONT"
                          );

  # These need to match the information in DEMStateBits.h
  @dem_master_bit_names = ( "RUN", "RSA", "PWRC EARLY EXIT", "SLEEP EXIT",
                              "READY", "SLEEP" );
  @time_master_bit_names = ( "TIME PENDING" );
  @dem_slave_bit_names = ( "RUN", "PWRC", "PWRC DELAY", "PWRC EARLY EXIT",
                           "WFPI", "SLEEP", "SLEEP_EXIT", "MSGS_REDUCED",
                           "RESET", "PWRC SUSPEND", "TIME REQUEST",
                           "TIME POLL", "TIME INIT" );

  # These need to match the information in DEMSlave.h
  @dem_slave_states = ( "INIT", "RUN", "SLEEP PREPARE", "SLEEP WAIT",
                        "SLEEP EXIT", "RUN PENDING", "POWER COLLAPSE",
                        "CHECK INTERRUPTS", "SWFI", "WFPI", "EARLY EXIT",
                        "RESET RECOVER", "RESET ACKNOWLEDGE", "ERROR" );

  # These need to match the information in DEMMaster.c
  @dem_master_states = ( "INIT", "RUN", "SLEEP WAIT", "SLEEP CONFIRMED",
                         "SLEEP EXIT", "RSA", "EARLY EXIT", "RSA DELAYED",
                         "RSA CHECK INTS", "RSA CONFIRMED", "RSA WAKING",
                         "RSA RESTORE", "RESET" );

  @dem_event_names = (  "NOT USED", "SMSM ISR", "STATE CHANGE",
                        "STATE_MACHINE_ENTER", "ENTER SLEEP", "END SLEEP",
                        "SETUP SLEEP", "SETUP POWER COLLAPSE", "SETUP SUSPEND",
                        "EARLY EXIT", "WAKEUP REASON", "DETECT WAKEUP",
                        "DETECT RESET", "DETECT SLEEPEXIT", "DETECT RUN",
                        "APPS SWFI", "SEND WAKEUP", "ASSERT OKTS",
                        "NEGATE OKTS", "PROC COMM CMD", "REMOVE PROC PWR",
                        "RESTORE PROC PWR", "SMI CLK DISABLED",
                        "SMI CLK ENABLED", "MAO INTS", "APPS WAKEUP INT",
                        "PROC WAKEUP", "PROC POWERUP", "TIMER EXPIRED",
                        "SEND BATTERY INFO", "TIME SYNC START",
                        "TIME SYNC SEND VALUE", "TIME SYNC DONE",
                        "TIME SYNC REQUEST", "TIME SYNC POLL", "TIME SYNC INIT",
                        "REMOTE PWR CB", "INIT"
                     );

  @dem_umts_event_names = ( "GL1_GO_TO_SLEEP", "GL1_SLEEP_START",
                            "GL1_AFTER_GSM_CLK_ON", "GL1_BEFORE_RF_ON",
                            "GL1_AFTER_RF_ON", "GL1_FRAME_TICK", 
                            "GL1_WCDMA_START",
                            "GL1_WCDMA_ENDING", "UMTS_NOT_OKTS", 
                            "UMTS_START_TCXO_SHUTDOWN", 
                            "UMTS_END_TCXO_SHUTDOWN",
                            "UMTS_START_ARM_HALT", "UMTS_END_ARM_HALT",
                            "UMTS_NEXT_WAKEUP_SCLK"
                          );

  $DEM_PRINT_TABLE = [ "NOT USED ",
                       "SMSM ISR",
                       "STATE CHANGE",
                       "STATE_MACHINE_ENTER",
                       "ENTER SLEEP",
                       "END SLEEP",
                       "SETUP SLEEP",
                       "SETUP POWER COLLAPSE",
                       "SETUP SUSPEND",
                       "EARLY EXIT",
                       "WAKEUP REASON",
                       "DETECT WAKEUP",
                       "DETECT RESET",
                       "DETECT SLEEPEXIT",
                       "DETECT RUN",
                       "APPS SWFI",
                       "SEND WAKEUP",
                       "ASSERT OKTS",
                       "NEGATE OKTS",
                       "PROC COMM CMD",
                       "REMOVE PROC PWR",
                       "RESTORE PROC PWR",
                       "SMI CLK DISABLED",
                       "SMI CLK ENABLED",
                       "MAO INTS",
                       "APPS WAKEUP INT",
                       "PROC WAKEUP",
                       "PROC POWERUP",
                       "TIMER EXPIRED",
                       "SEND BATTERY INFO",
                       "TIME SYNC START",
                       "TIME SYNC SEND VALUE",
                       "TIME SYNC DONE",
                       "TIME SYNC REQUEST",
                       "TIME SYNC POLL",
                       "TIME SYNC INIT",
                       "REMOTE PWR CB",
                       "DEM INIT"
                     ];
} # BEGIN

# Return an array of integers that represent bits set in this bitfield.
sub get_bits {
  @bits = ();
  for($i = 0; $i < 32; $i++) {
    if($_[0] & (1 << $i)) {
      push(@bits, $i);
    }
  }
  return @bits;
}

sub dem_print {
  my( $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$DEM_PRINT_TABLE[$event];
  my $umts_cntrl = ($event & $DEM_UMTS_BASE) &&
                   ( ($event - $DEM_UMTS_BASE) < scalar(dem_umts_event_names) );
  my $tmp;

  if( $cntrl eq "SMSM ISR" ) {
     printf("DEM: %s: entry: %s ", $dem_event_names[$event],
            $smsm_entry_names[$d1]);
     $tmp = ($d2 ^ $d3) & $d2;
     printf("cleared bits: ");
     if($d1 >= 0 && $d1 < 3) {
       print join(' ', map($smsm_state_bit_names[$_], get_bits($tmp)));
     }
     elsif($d1 >= 3 && $d1 < 6) {
       print join(' ', map($dem_slave_bit_names[$_], get_bits($tmp)));
     }
     elsif($d1 == 6) {
       print join(' ', map($dem_master_bit_names[$_], get_bits($tmp)));
     }
     elsif($d1 == 7) {
       print join(' ', map($time_master_bit_names[$_], get_bits($tmp)));
     }
     $tmp = ($d2 ^ $d3) & $d3;
     printf(" set bits: ");
     if($d1 >= 0 && $d1 < 3) {
       print join(' ', map($smsm_state_bit_names[$_], get_bits($tmp)));
     }
     elsif($d1 >= 3 && $d1 < 6) {
       print join(' ', map($dem_slave_bit_names[$_], get_bits($tmp)));
     }
     elsif($d1 == 6) {
       print join(' ', map($dem_master_bit_names[$_], get_bits($tmp)));
     }
     elsif($d1 == 7) {
       print join(' ', map($time_master_bit_names[$_], get_bits($tmp)));
     }
     printf("\n");
  }
  elsif( $cntrl eq "STATE CHANGE" ) {
    if($d1 == 0) {
      # Slave state change.
      printf("DEM: %s: slave leaving %s and entering %s\n",
             $dem_event_names[$event], $dem_slave_states[$d2],
             $dem_slave_states[$d3]);
    }
    else {
      # Master state change.
      printf("DEM: %s: master leaving %s and entering %s\n",
             $dem_event_names[$event], $dem_master_states[$d2],
             $dem_master_states[$d3]);
    }
  }
  elsif( $cntrl eq "APPS WAKEUP INT" ) {
     printf( "DEM: %s  %s   %08x   %08x\n",
            $dem_event_names[$event], $dem_shared_int_names[$d1], $d2, $d3 );
  }
  elsif( $cntrl eq "REMOVE PROC PWR" ) {
     printf( "DEM: %s from %s\n", $dem_event_names[$event], $dem_cpu_names[$d1]);
  }
  elsif( $cntrl eq "RESTORE PROC PWR" ) {
     printf( "DEM: %s to %s\n", $dem_event_names[$event], $dem_cpu_names[$d1]);
  }
  elsif( $cntrl eq "PROC WAKEUP" ) {
    printf("DEM: %s: proc: %s reason: ", $dem_event_names[$event], $dem_cpu_names[$d1]);
    print join(' ', map($dem_wakeup_reason_names[$_], get_bits($d2)));
    if ( $d3 == 0 ) {
      printf(" <not handled>\n");
    } else {
      printf(" <handled>\n");
    }
  }
  elsif( $cntrl eq "PROC POWERUP" ) {
    printf("DEM: %s: proc: %s reason: ", $dem_event_names[$event], $dem_cpu_names[$d1]);
    print join(' ', map($dem_wakeup_reason_names[$_], get_bits($d2)));
      if ( $d3 == 0 ) {
        printf(" <not handled>\n");
      } else {
        printf(" <handled>\n");
      }
  }
  elsif ( $cntrl eq "WAKEUP REASON" ) {
    printf( "DEM: %s: ", $dem_event_names[$event] );

    $tmp = 0;
    while ( $d1 ) {
      if ( $d1 & 0x00000001 ) {
        # This bit is set, so print the string for it.
        printf( "%s ", $dem_wakeup_reason_names[$tmp] );

        if ( $dem_wakeup_reason_names[$tmp] eq "SMD" )
        {
          # Print the SMD wakeup info
          my $prog = $$RPC_PROG_NAMES{$d2};

          if( defined $prog ) {
            printf( " - RPC Prog: $prog, RPC Proc: %d", $d3 );
          }
          else {
            printf( " - RPC Prog: 0x%x, RPC Proc: %d", $d2, $d3 );
          }
        }
        elsif ( $dem_wakeup_reason_names[$tmp] eq "INT" )
        {
          # Print the INT wakeup info
          printf( " - Ints pending mask: 0x%x", $d2 );
        }
        elsif ( $dem_wakeup_reason_names[$tmp] eq "GPIO" )
        {
          # Print the GPIO wakeup info
          printf( " - GPIO number: %d", $d2 );
        }
      }
      $d1 >>= 1;
      $tmp++;
    }
    printf( "\n" );
  }
  elsif( $cntrl eq "TIME SYNC START" ) {
    printf( "DEM: %s: servicing ", $dem_event_names[$event] );
    print join(' ', map($dem_cpu_names[$_], get_bits($d1)));
    printf( "\n" );
  }
  elsif( $cntrl eq "TIME SYNC SEND VALUE" ) {
    printf( "DEM: %s: sclk of %i to ", $dem_event_names[$event], $d2 );
    print join(' ', map($dem_cpu_names[$_], get_bits($d1)));
    printf( "\n" );
  }
  elsif( $cntrl eq "TIME SYNC REQUEST" ||
         $cntrl eq "TIME SYNC POLL" ) {
    printf( "DEM: %s from %s\n", $dem_event_names[$event], $dem_cpu_names[$d1] );
  }
  elsif( $cntrl eq "TIME SYNC INIT" ) {
    printf( "DEM: %s from %s, offset was %i\n", $dem_event_names[$event], $dem_cpu_names[$d1], $d2 );
  }
  elsif( $cntrl eq "DEM INIT" ) {
    printf( "DEM: %s: DEM task priority: %d  PWRC warmup time: %2.2f ms\n", $dem_event_names[$event], $d1, ( $d2 / 32768 ) * 1000 );
  }
  elsif( defined $cntrl ) {
    printf( "DEM: %s  %08x   %08x   %08x\n",
            $dem_event_names[$event], $d1, $d2, $d3 );
  }
  elsif( defined $umts_cntrl  ) {
    printf("DEM: %s  %08x   %08x   %08x\n",
            $dem_umts_event_names[$event - $DEM_UMTS_BASE], $d1, $d2, $d3 );
  }
  else {
    generic_print( $DEM_PRINT_TABLE, "DEM", @_ );
  }
} # dem_print

###########################################################################
#        SLEEP Definitions and print routines
###########################################################################
BEGIN {        # Use begin block so that these variables are defined at runtime

  @sleep_event_names = (  "NOT USED", "NO SLEEP OLD", "INSUF TIME", 
                          "UART CLOCK", "SLEEP INFO", 
                          "TCXO END", "ENTER TCXO", "NO SLEEP",
                          "RESOURCE ACQUIRED", "RESOURCE RELEASED"
                       );

  @sleep_resource_names = ( "MICROPROC_SLEEP", "TCXO", "MEMORY",
                            "VDD_MIN", "APPS_SLEEP", "APPS_PWRC" );


  $SLEEP_PRINT_TABLE = [ "NOT USED ",
			 "NO SLEEP OLD", 
			 "INSUF TIME", 
			 "UART CLOCK ", 
			 "SLEEP INFO ", 
			 "TCXO END ", 
			 "ENTER TCXO ",
			 "NO SLEEP",
			 "RESOURCE ACQUIRED",
			 "RESOURCE RELEASED"
		       ];
} # BEGIN

sub sleep_print {
  my( $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$SLEEP_PRINT_TABLE[$event];

  if( $cntrl eq "NO SLEEP OLD" ) {
    printf( "SLEEP:  %s  MASK = %08x\n",
            $sleep_event_names[$event], $d1 );
  }

  elsif( $cntrl eq "NO SLEEP" ) {
    my $mod_not_okts, $apps_not_okts, $i;
    $mod_not_okts = $apps_not_okts = '';

    my $not_okts_mask = (($d2 << 32) | $d3 );

    $i = 0;
    while( $not_okts_mask ) {

      # If bit is set and we are on the apps processor, save voter name.
      # d1=1 Apps  d1=0 Modem
      if ( $not_okts_mask & 0x1 )
      {
        if( $d1 ) {
          $apps_not_okts .= $$apps_voters[ $i ] . ' ';
        }
        else {
          $mod_not_okts .= $$mod_voters[ $i ] . ' ';
        }
      }

      $i++;
      $not_okts_mask >>= 1;
    }
    if( $d1 ) {
      printf( "SLEEP:  %s - %s\n", 
              $sleep_event_names[$event], $apps_not_okts );
    }
    else {
      printf( "SLEEP:  %s - %s\n", 
      $sleep_event_names[$event], $mod_not_okts );
    }
  }
  elsif( $cntrl eq "RESOURCE ACQUIRED" ) {
    printf( "SLEEP: %s: resources ", $sleep_event_names[$event] );
    print join(' ', map($sleep_resource_names[$_], get_bits($d1)));
    if(($id & 0xC0000000) == 0x80000000) {
      # This message is from apps
      printf( " by %s", $$apps_voters[$d2] );
    }
    elsif(($id & 0xC0000000) == 0x0) {
      # This message is from modem
      printf( " by %s", $$mod_voters[$d2] );
    }
    printf("\n");
  }
  elsif( $cntrl eq "RESOURCE RELEASED" ) {
    printf( "SLEEP: %s: resources ", $sleep_event_names[$event] );
    print join(' ', map($sleep_resource_names[$_], get_bits($d1)));
    if(($id & 0xC0000000) == 0x80000000) {
      # This message is from apps
      printf( " last voter %s", $$apps_voters[$d2] );
    }
    elsif(($id & 0xC0000000) == 0x0) {
      # This message is from modem
      printf( " last voter %s", $$mod_voters[$d2] );
    }
    printf("\n");
  }
  elsif( defined $cntrl ) {
    printf( "SLEEP:  %s  %08x   %08x   %08x\n",
            $sleep_event_names[$event], $d1, $d2, $d3 );
  }
  else {
    generic_print( $SLEEP_PRINT_TABLE, "SLEEP", @_ );
  }
} # sleep_print


###########################################################################
#        DCVS print routines
###########################################################################
BEGIN {        # Use begin block so that these variables are defined at runtime
  @DCVS_EVENT_NAMES =( "IDLE ",
                       "ERR  ",
                       "CHG  ",
                       "REG  ",
                       "DEREG" );
  
  $DCVS_PRINT_TABLE =[ "IDLE",
                       "ERR",
                       "CHG",
                       "REG",
                       "DEREG" ];
} # BEGIN

sub dcvs_print {
  my($id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$DCVS_PRINT_TABLE[$event];
  
  if( defined $cntrl ) {
    printf( "DCVS:   %s     %d     %08x     %d\n", 
            $DCVS_EVENT_NAMES[$event], $d1, $d2, $d3 );
  }
  else {
    generic_print( $DCVS_PRINT_TABLE, "DCVS", @_ );
  }
} # dcvs_print

###########################################################################
#        RPC ROUTER Definitions and print routines
###########################################################################
BEGIN {        # Use begin block so that these variables are defined at runtime
  @ROUTER_PRINT_TABLE = ( "UNKNOWN",
                          "READ   ",
                          "WRITTEN",
                          "CNF REQ",
                          "CNF SNT",
                          "MID READ",
                          "MID WRITTEN",
                          "MID CNF REQ",
                          "PING",
                          "SERVER PENDING",
                          "SERVER REGISTERED");
} # BEGIN
sub rpc_router_print {

  my($id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $ROUTER_PRINT_TABLE[$event];

  if( ($cntrl eq "CNF REQ") or ($cntrl eq "CNF SNT")){
    printf( "ROUTER: %s pid = %08x    cid = %08x    tid = %08x\n", 
             $cntrl, $d1, $d2, $d3 );
  }
  elsif( $cntrl eq "MID READ"){
    printf( "ROUTER: READ    mid = %08x    cid = %08x    tid = %08x\n", 
             $d1, $d2, $d3 );
  }
  elsif( $cntrl eq "MID WRITTEN"){
    printf( "ROUTER: WRITTEN mid = %08x    cid = %08x    tid = %08x\n", 
             $d1, $d2, $d3 );
  }
  elsif( $cntrl eq "MID CNF REQ"){
    printf( "ROUTER: CNF REQ pid = %08x    cid = %08x    tid = %08x\n", 
             $d1, $d2, $d3 );
  }
  elsif( $cntrl eq "PING"){
     printf( "ROUTER: PING    pid = %08x    cid = %08x    tid = %08x\n", 
             $d1, $d2, $d3 );
  }
  elsif( $ctrl eq "SERVER PENDING"){
    printf("ROUTER: SERVER PENDING REGISTRATION    prog = 0x%08x vers=0x%08x tid = %08x",
            $d1, $d2, $d3  );
  }
  elsif ($ctrl eq "SERVER REGISTERED") {
    printf("ROUTER: PENDING SERVER REGISTERED      prog = 0x%08x vers=0x%08x tid = %08x",
            $d1, $d2, $d3  );
  }
  elsif( defined $cntrl ) {
    printf( "ROUTER: %s xid = %08x    cid = %08x    tid = %08x\n", 
              $cntrl, $d1, $d2, $d3 );
  } 
  else {
    generic_print( $ROUTER_PRINT_TABLE, "ROUTER:   ", @_ );
  }
} # rpc_router_print

###########################################################################
#        Error Definitions and print routines
###########################################################################

BEGIN {        # Use begin block so that these variables are defined at runtime
  $ERR_PRINT_TABLE = [ "UNUSED",
                       EVENT_FATAL,
                       EVENT_FATAL_TASK ];

  $ERR_DATA1 = "....";
  $ERR_DATA2 = "....";
  $ERR_DATA3 = "....";
} # BEGIN

sub err_print {
  my( $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$ERR_PRINT_TABLE[$event];

  if( $cntrl eq EVENT_FATAL ) {
    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First ERR record
      printf( "ERR: FATAL " );
      $ERR_DATA1 = $d1;
      $ERR_DATA2 = $d2;
      $ERR_DATA3 = $d3;
    }
    else {
      # Second ERR record
      my $name = pack( "I5", $ERR_DATA1, $ERR_DATA2, $ERR_DATA3, $d1, $d2 );
      $name =~ s/\0.*//;
      print " file: \"$name\" line: $d3\n";
    }
  }
  elsif( $cntrl eq EVENT_FATAL_TASK ) {
    if( ($id & $CONTINUE_MASK) eq 0 ) {
      # First ERR record
      printf( "ERR: FATAL " );
      $ERR_DATA1 = $d1;
      $ERR_DATA2 = $d2;
      $ERR_DATA3 = $d3;
    }
    else {
      # Second ERR record
      my $name = pack( "I5", $ERR_DATA1, $ERR_DATA2, $ERR_DATA3, $d1, $d2 );
      $name =~ s/\0.*//;
      printf( " task: \"$name\" tcb addr: 0x%08x\n", $d3 );
    }
  }
  else {
    generic_print( $ERR_PRINT_TABLE, "ERR", @_ );
  }
} # err_print

###########################################################################
#        CLKRGM Definitions and print routines
###########################################################################

BEGIN {        # Use begin block so that these variables are defined at runtime
  $CLKRGM_PRINT_TABLE = [ "PLL_ON",
                        "PLL_OFF",
                        "CXO_ON",
                        "CXO_OFF",
                        "PXO_ON",
                        "PXO_OFF",
                        "ACPU_PWRCLPS",
                        ACPU_RESTORE,
                        "APC_RAIL_OFF",
                        "APC_RAIL_ON",
                        SET_MSMC1,
                        SET_MSMC2,
                        SWITCH_MCPU,
                        "SWITCH_SYSBUS",
                        SWITCH_ACPU ];
} # BEGIN

sub clkrgm_print {
  my( $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$CLKRGM_PRINT_TABLE[$event];

  if( $cntrl eq SWITCH_MCPU ) {
    printf( "CLKRGM: SWITCH_MCPU - MCPU speed - new = %d, old = %d \n", $d1, $d2);
  }
  elsif( $cntrl eq SWITCH_ACPU ) {
    printf( "CLKRGM: SWITCH_ACPU - ACPU speed - new = %d, old = %d \n", $d1, $d2);
  }
  elsif( $cntrl eq SET_MSMC1 ) {
    printf( "CLKRGM: SET_MSMC1 - voltage - new = %d, old = %d \n", $d1, $d2);
  }
  elsif( $cntrl eq SET_MSMC2 ) {
    printf( "CLKRGM: SET_MSMC2 - voltage - new = %d, old = %d \n", $d1, $d2);
  }
  elsif( $cntrl eq ACPU_RESTORE ) {
    printf( "CLKRGM: ACPU_RESTORE - current perf level = %d \n", $d1);
  }
  else  {
    generic_print( $CLKRGM_PRINT_TABLE, "CLKRGM", @_ );
  }
} # clkrgm_print


###########################################################################
#        Generic and common print routines
###########################################################################
my $TMC_PRINT_TABLE =
    [ "WAIT",
      "GOTO INIT",
      "BOTH INIT" ];

my $TIMETICK_PRINT_TABLE =
    [ "START",
      "GOTO WAIT",
      "GOTO INIT" ];

sub generic_print {
  my( $table, $name, $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;
  my $cntrl = $$table[$event];
  
  if( defined $cntrl ) {
      printf( "$name: $cntrl\n", $d1, $d2, $d3 );
  }
  else {
    default_print( @_ );
  }
} # generic_print

sub print_line_header {
  my( $proc_flag, $time, $ticks ) = @_;

  if( $ticks eq TRUE ) {
    if ( $proc_flag eq 0x80000000 ) {
      $LINE_HEADER = sprintf( "APPS: 0x%08x    ",  $time );
    }
    elsif ( $proc_flag eq 0x40000000 ) {
      $LINE_HEADER = sprintf( "QDSP: 0x%08x    ",  $time );
    }
    else {
      $LINE_HEADER = sprintf( "MODM: 0x%08x    ", $time );
    }
  }
  else {
    # Convert time from ticks to seconds
    $time = $time / 32000;
        
    if ( $proc_flag eq 0x80000000 ) {
      $LINE_HEADER = sprintf( "APPS: %11.6f    ",  $time );
    }
    elsif ( $proc_flag eq 0x40000000 ) {
      $LINE_HEADER = sprintf( "QDSP: %10d    ",  $time );
    }
    else {
      $LINE_HEADER = sprintf( "MODM: %11.6f    ", $time );
    }
  }

  print $LINE_HEADER;
} # print_line_header
  
sub default_print {
  shift @_;                     # remove table
  printf( "%s: %08x    %08x    %08x    %08x\n", @_ );
} # default_print

sub debug_print {
  my( $id, $d1, $d2, $d3 ) = @_;
  my $event = $id & $LSB_MASK;

  if( $event eq 100 ) {
    printf( "PROXY: dispatch to %08x, queue len = %d\n", $d1, $d2 );
  }
  else {
    default_print( undef, "DEBUG", @_ );
  }
} #debug_print

sub print_record {
  my( $rec ) = @_;
  my $id = $$rec[0] & $BASE_MASK;

  if( $id == $SMEM_LOG_DEBUG_EVENT_BASE ) {
    debug_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif ( $id == $SMEM_LOG_ONCRPC_EVENT_BASE )
  {
    oncrpc_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_LOG_SMEM_EVENT_BASE ) {
    smem_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_LOG_TMC_EVENT_BASE) {
    generic_print( $TMC_PRINT_TABLE, "TMC",
                   $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_LOG_TIMETICK_EVENT_BASE) {
    generic_print( $TIMETICK_PRINT_TABLE, "TIMETICK",
                   $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_DEM_EVENT_BASE) {
    dem_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_ERR_EVENT_BASE) {
    err_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_LOG_DCVS_EVENT_BASE) {
    dcvs_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_LOG_SLEEP_EVENT_BASE) {
    sleep_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif ( $id == $SMEM_LOG_RPC_ROUTER_EVENT_BASE )
  {
    rpc_router_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  elsif( $id == $SMEM_LOG_CLKREGIM_EVENT_BASE) {
    clkrgm_print( $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
  else {
    # Unknown event base just do "default" print
    default_print( undef, "UNKNOWN", $$rec[0], $$rec[2], $$rec[3], $$rec[4] );
  }
} # print_record

sub main {
  my( $relative_time, $ticks ) = parse_args( @ARGV );
  my $base_time;

  if (    ( -e "smem_log_static_events.lst" )
       && ( -e "smem_log_static_index.lst" )
       && ( -e "smem_log_sleep_voters.lst" ) )
  {
    my $static_log_events = parse_log_file( "smem_log_static_events.lst" );
    my $static_index = parse_index_file( "smem_log_static_index.lst");

    $base_time = 0;

    # These pointers must be global so sleep_print() can access them.
    ($mod_voters, $apps_voters) = parse_voter_file( "smem_log_sleep_voters.lst" );

    # Print out the registered sleep voters 
    print "REGISTERED SLEEP VOTERS:" .
           "\nMODM: " . join(' ', @$mod_voters) .
           "\nAPPS: " . join(' ', @$apps_voters) .
           "\n";
  
  
    print "==============================\n" .
          "        STATIC LOG            \n" .
          "==============================\n" ;
    #Parse Static log
    my $static_idx = 0;
    while( $static_idx < $static_index ) {
      my $rec = $$static_log_events[$static_idx];
  
      if( $$rec[0] != 0 ) {
        if( $relative_time eq TRUE ) {
          $base_time = $$rec[1];
          $relative_time = FALSE;
        }
  
        if( ($$rec[0] & $CONTINUE_MASK) eq 0 ) {
          # Not continuation event, print header
          print_line_header( $$rec[0] & 0xC0000000, # modem/apps flag
                             $$rec[1] - $base_time, # time
                             $ticks );              # seconds/ticks flag
        }
        print_record( $rec );
      }
      $static_idx++;
    }
    if( ($static_index >= $MAX_STATIC_LENGTH) && (@$static_log_events) ) {
      print "*** STATIC LOG FULL - " .
            "Remaining entries will be in CIRCULAR LOG ***\n";
    }
  
    print "==============================\n" .
          "        CIRCULAR LOG          \n" .
          "==============================\n\n" ;

  }
  else
  {
    print "Static log not detected, parsing circular log ...";
  }

  print_circular_log("smem_log_events.lst", "smem_log_index.lst",
	             $relative_time, $ticks);

  if(-f "smem_log_power_events.lst" && -f "smem_log_power_index.lst")
  {
    print "==============================\n" .
          "      POWER CIRCULAR LOG      \n" .
          "==============================\n\n" ;
    print_circular_log("smem_log_power_events.lst", "smem_log_power_index.lst",
                       $relative_time, $ticks);
  }
} # main

sub print_circular_log {
  my $log_events    = parse_log_file  ( shift );
  my $index         = parse_index_file( shift );
  my $relative_time = shift;
  my $ticks         = shift;

  my $idx;
  my $base_time  = 0;

  # Suppress empty logs
  return unless @$log_events;

  for( $idx = $index; $idx < $MAX_LENGTH; $idx++ ) {
    my $rec = $$log_events[$idx];

    if( $$rec[0] != 0 ) {
      if( $relative_time eq TRUE ) {
        $base_time = $$rec[1];
        $relative_time = FALSE;
      }

      if( ($$rec[0] & $CONTINUE_MASK) eq 0 ) {
        # Not continuation event, print header
        print_line_header( $$rec[0] & 0xC0000000, # modem/apps/q6 flag
                           $$rec[1] - $base_time, # time
                           $ticks );              # seconds/ticks flag
      }

      print_record( $rec );
    }
  }

  for(  $idx = 0; $idx < $index; $idx++ ) {
    my $rec = $$log_events[$idx];

    if( $$rec[0] != 0 ) {
      if( $relative_time eq TRUE ) {
        $base_time = $$rec[1];
        $relative_time = FALSE;
      }

      if( ($$rec[0] & $CONTINUE_MASK) eq 0 ) {
        # Not continuation event, print header
        print_line_header( $$rec[0] & 0xC0000000, # modem/apps/q6 flag
                           $$rec[1] - $base_time, # time
                           $ticks );              # seconds/ticks flag
      }

      print_record( $rec );
    }
  }
} # print_circular_log
