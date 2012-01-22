#!/usr/bin/perl -Xw

#****************************************************************************
#**
#**         smem_sleep_stats.pl
#**
#**  Filters smem log file to determine sleep statistics.  Always writes to 
#** standard output.
#**
#** 
#** Copyright (c) 2006 by QUALCOMM, Incorporated.  All Rights Reserved.
#****************************************************************************
#**
#**                        EDIT HISTORY FOR MODULE
#**
#**  $Header: //source/qcom/qct/core/pkg/2H09/halcyon_modem/main/latest/AMSS/products/7x30/tools/debug/smem_sleep_stats.pl#9 $
#**
#** when       who     what, where, why
#** --------   ---     ------------------------------------------------------
#**  8/21/07   bjs     Fixed parsing of TCXO Shutdown when not in Power Collapse
#**                    Also fixed TCXO parsing when a Power Collapse or Apps Sleep 
#**                    occurs without a TCXO Shutdown.
#**         
#****************************************************************************

main();
exit;

#****************************************************************************
# FUNCTION print_usage
#
# DESCRIPTION
#    Prints error message and usage information, then exits.
#
# PARAMETERS
#    Optional error message.
#
# RETURN VALUE
#    None - never returns
#****************************************************************************
sub print_usage {
  my( $error ) = @_;

  if( defined $error ) {
    print STDERR "ERROR: $error\n\n";
  }

  print STDERR "usage: smem_sleep_stats.pl [<smem_log_file>] \n";
  print STDERR "       Filters and processes sleep statistics from the\n";
  print STDERR "       log file  Writes to standard output\n";
  print STDERR "\n";
  print STDERR "       -h     : print this message\n";
  print STDERR "       --help : print this message\n";

  exit;
} # print_usage

#****************************************************************************
# FUNCTION parse_args
#
# DESCRIPTION
#    Parses the command line args looking for an optional file name and
#    options starting with "-". Calls print_usage when an error is detected
#
# RETURN VALUE
#    The file name or undef if no file name was given
#****************************************************************************
sub parse_args {
  my @args = @ARGV;
  my $filename = pop @args;

  return if not defined $filename;
  print_usage if $filename eq "-h" or $filename eq "--help";

  if( $filename =~ m/^-/ ) {
    push @args, $filename;
    undef $filename;
  }

  $opt = shift @args;

  while( defined $opt ) {
    if( $opt eq "--help" or $opt eq "-h" ) {
      print_usage();
      # Just in case print_usage returns
      $opt = shift @args;
      next;
    }

    print_usage( "unknown option $opt" );
  }

  return $filename
} # parse_args

#****************************************************************************
# FUNCTION read_log
#
# DESCRIPTION
#    Reads the lines from the given file name or from standard in and returns
#    a reference to an array of the lines. Also removes the line endings.
#
# PARAMETERS
#    Optional file name
#
# RETURN VALUE
#    Reference to array of lines.
#****************************************************************************
sub read_log {
  my( $filename ) = @_;
  my @lines;

  if( defined $filename ) {
    open FD, "<$filename" or die "unable to open \"$filename\"";

    while( <FD> ) {
      # delete end-of-line chars
      s/[\r\n]+//;
      push @lines, ($_);
    }

    close FD;
  }
  else {
     print_usage();
  }

  return \@lines;
} # read_log

sub read_timestamp
{
  my ($line) = @_;

  return substr($line, 5, 12) + 0;
}

sub calc_stats   #(@aStart, @aStop);
{
  local ($startR, $stopR, $reasonR) = @_;

  #Calculate the average
  $sum = 0;
  $max = 0;
  for ($index=0;$index<=@$startR-1;$index++)
  {
    $diff = (@$stopR[$index]-@$startR[$index]);
    $sum += $diff;
    if ($diff > $max)
    {
      $max = $diff;
    }
  }
  $avg = $sum / (@$startR+0);

  #Calculate the Standard Deviation
  $stddev = 0;
  for ($index=0;$index<=@$startR-1;$index++)
  {
    $diff = @$stopR[$index]-@$startR[$index];
    $stddev += ($diff * $diff);
  }
  $stddev -= ($avg * $avg * (@$startR+0));
  $stddev /= (@$startR+0);
  $stddev = sqrt($stddev);

  $reasonPct = 0;
  for ($index=0;$index<=@$startR-1;$index++)
  {
    if( @$reasonR[$index] =~ m/PC TIMER EXPIRED/ ) 
    {
      $reasonPct++;
    }
  }
  $reasonPct = ($reasonPct / @$startR) * 100;

  return($avg, $max, $stddev, $reasonPct);
}

sub displayOutput
{
  local ($startR, $stopR, $reasonR, $sleepR, $addrR, $setR, $calcR) = @_;

  printf("%-12s  %-12s  %-20s  %-11s  %-8s  %-11s\n", 
                 "Start",
                 "Stop",
                 "Reason",
                 "Tmr Val(ms)",
# Since these features require changes in the code, they
# are disabled for now.
#                 "Tmr Addr",
#                 "Tmr Set(ms)"
                 );

  for ($index=0;$index<=@$startR-1;$index++)
  {
# See the comment above.
#    printf("%12.6f  %12.6f  %-20s  %-11u  %08X  %-11u\n", 
    printf("%12.6f  %12.6f  %-20s  %-11u\n", 
                   @$startR[$index],
                   @$stopR[$index],
                   @$reasonR[$index],
                   @$sleepR[$index],
# See the comment above.
#                   @$addrR[$index],
#                   @$setR[$index]
                   );
  }
  printf ("\n");
  printf("Time Stats:  Avg: %10.4f  Max: %10.4f  StdDev: %10.4f\n", 
                 @$calcR[0],
                 @$calcR[1],
                 @$calcR[2]);
  printf("Reason Stats:  PC TIMER EXPIRED %%: %4.1f\n", 
                 @$calcR[3]);
}

#****************************************************************************
# FUNCTION main
#
# DESCRIPTION
#    Reads the lines from the given file name or from standard in, filters
#    out matching RPC call and return lines and prints the remaining lines.
#
# PARAMETERS
#    Optional file name
#
# RETURN VALUE
#    None
#****************************************************************************
sub main {
  my $filename = parse_args;
  my $modemState = 0;
  my $modemExitReason;
  my $modemStartTime;
  my $modemStopTime;
  my $modemTimerSleepTime;
  my $modemTimerAddress;
  my $modemTimerSet;

  my $appsState = 0;
  my $appsExitReason;
  my $appsStartTime;
  my $appsStopTime;
  my $appsTimerSleepTime;
  my $appsTimerAddress;
  my $appsTimerSet;

  my $lines = read_log( $filename );
  my $num_lines = scalar @$lines;

  # Check each input line, if it's a reply add an entry to the replies hash
  # using the processor associated with the call and the xid as the key.
  foreach $line (@$lines) 
  {
    # Initial modemState.  Searching for start of Power Collapse.
    #
    if($modemState==0)
    {
      # Search for 
      # MODM:   61.726438    DEM: PWRCLPS APPS  00000000   00000000   00000000
      # or
      # APPS:   58.565969    DEM: SETUP APPS PWRCLPS  00007d1e   000003d1   00000013
      # or
      # APPS:  234.454031    DEM: ENTER SLEEP  00003000   000000a8   00000001
      if( $line =~ m/^MODM.*DEM.*PWRCLPS APPS/ ) 
      {
        $modemState = 1;
      }
      elsif( $line =~ m/^APPS.*DEM.*ENTER SLEEP/ ) 
      {
        # Record the Sleep setup values.
        #
        $modemTimerSleepTime = (hex(substr($line, 40, 8)) * 1000 / 32768);
        $modemTimerAddress = hex(substr($line, 51, 8));
        $modemTimerSet = (hex(substr($line, 62, 8)) * 1000 / 32768);
        $modemState = 1;
      }
      elsif( $line =~ m/^APPS.*DEM.*SETUP APPS PWRCLPS/ ) 
      {
        # Record the Power Collapse setup values.
        #
        $modemTimerSleepTime = (hex(substr($line, 46, 8)) * 1000 / 32768);
        $modemTimerAddress = hex(substr($line, 57, 8));
        $modemTimerSet = (hex(substr($line, 68, 8)) * 1000 / 32768);
      }

    }
    elsif ($modemState==1)
    {
      # Search for the beginning of the TCXO Shutdown
      # MODM:   61.727312    DEM: ENTER TCXO  00007b3e   00000000   00000000
      # MODM:  195.137188    SLEEP:  ENTER TCXO  00014381   00000000   00000000
      if(( $line =~ m/^MODM.*DEM.*ENTER TCXO/ ) ||
         ( $line =~ m/^MODM.*SLEEP.*ENTER TCXO/ ))
      {
        # We are entering TCXO Shutdown
        $modemState = 2;

        # Capture the timestamp.
        $modemStartTime = read_timestamp($line);
      }
      # If we get either of these, then we have exited sleep without a tcxo shutdown,
      # and should go back to state 0.
      # APPS:  247.910000    DEM: END APPS SLEEP  00000002   00000000   00000000
      # or
      # MODM:   62.715344    DEM: RESTORE APPS PWR  00000000   00000000   00000000
      elsif( $line =~ m/^APPS.*DEM.*END APPS SLEEP/ ) 
      {
        $modemState = 0;
      }
      elsif( $line =~ m/^MODM.*DEM.*RESTORE APPS PWR/ ) 
      {
        $modemState = 0;
      }
    }
    elsif ($modemState==2)
    {
      # Search for the end of the TCXO Shutdown
      # MODM:   58.762750    DEM: TCXO END  00000000   00000000   00000000
      # MODM:  197.722062    SLEEP:  TCXO END  00000000   00000000   00000000
      if(( $line =~ m/^MODM.*DEM.*TCXO END/ ) ||
         ( $line =~ m/^MODM.*SLEEP.*TCXO END/ )) 
      {
        # Capture the timestamp.
        $modemStopTime = read_timestamp($line);
        # We are trying to find the reason for waking up.
        $modemState = 3;
      }
    }
    elsif ($modemState==3)
    {
      # Determine the reason for waking up.  One of the below.
      #MODM:   58.770844    DEM: MAO INTS  00000001   00000000   00000000
      #MODM:   59.569250    DEM: PC TIMER EXPIRED  00000000   00000000   00000000
      #MODM:   61.392906    DEM: BATTERY INFO  00000002   00000000   00000000

      $reasonSt = index($line, "DEM: ") + 5;
      $reasonStp = index($line, "  ", $reasonSt);
      $modemExitReason = substr($line, $reasonSt, $reasonStp - $reasonSt);

      if( $line =~ m/MAO INTS/ ) 
      {
        # Find the reason for waking up.
        $modemState = 1;
      }
      else
      {
        $modemState = 0;
      }

      #Store the values into my arrays.
      push (@mStart, $modemStartTime);
      push (@mStop, $modemStopTime);
      push (@mReason, $modemExitReason);
      push (@mSleep, $modemTimerSleepTime);
      push (@mAddr, $modemTimerAddress);
      push (@mSet, $modemTimerSet);

    }
  }

  # Print results for Modem Processor.
  printf ("\nResults for Modem Processor\n\n");

  if ((@mStart+0) == 0)
  {
    printf("No TCXO shutdowns detected.\n");
  }
  else
  {
    # Calculate Statistics for Modem Processor.
    my @mCalc = calc_stats(\@mStart, \@mStop, \@mReason);

    displayOutput(\@mStart, \@mStop, \@mReason, \@mSleep, \@mAddr, \@mSet, \@mCalc);
  }

  # Determine Apps Power collapse values.
  #
  foreach $line (@$lines) 
  {
    # Initial State.  Searching for start of Power Collapse.
    #
    if($appsState==0)
    {
      $appsExitReason = "";

      # Search for 
      # MODM:   61.726438    DEM: PWRCLPS APPS  00000000   00000000   00000000
      # or
      # APPS:   58.565969    DEM: SETUP APPS PWRCLPS  00007d1e   000003d1   00000013
      if( $line =~ m/^MODM.*DEM.*PWRCLPS APPS/ ) 
      {
        $appsState = 1;
        # Capture the timestamp.
        $appsStartTime = read_timestamp($line);
      }
      elsif( $line =~ m/^APPS.*DEM.*SETUP APPS PWRCLPS/ ) 
      {
        # Record the apps Power Collapse setup values.
        #
        $appsTimerSleepTime = (hex(substr($line, 46, 8)) * 1000 / 32768);
        $appsTimerAddress = hex(substr($line, 57, 8));
        $appsTimerSet = (hex(substr($line, 68, 8)) * 1000 / 32768);
      }
    }
    elsif ($appsState==1)
    {
      # Now search for 
      # MODM:   61.384250    DEM: TCXO END  00000000   00000000   00000000
      # MODM:  234.446281    SLEEP:  TCXO END  00000000   00000000   00000000
      # MODM:  509.155500    DEM: NEGATE OKTS  00000003   00000000   00000000
      # or
      # MODM:   62.715344    DEM: RESTORE APPS PWR  00000000   00000000   00000000
      # or
      # MODM:  234.477531    ONCRPC: CALL    xid = 000002cb    proc =  15    prog = CM CB    task = CM
      if(( $line =~ m/^MODM.*DEM.*TCXO END/ ) ||
         ( $line =~ m/^MODM.*SLEEP.*TCXO END/ )) 
      {
        # the next line indicates the reason for exit.
        $appsState = 2;
      }
      elsif (( $line =~ m/^MODM.*DEM.*NEGATE OKTS/ ) &&
             (length($appsExitReason) == 0))
      {
        # if we have an exit reason from the TCXO END, use it.  Otherwise,
        # we use the line before NEGATE OKTS as our exit reason.
        $reasonSt = index($lastLine, "DEM: ") + 5;
        $reasonStp = index($lastLine, "  ", $reasonSt);
        $appsExitReason = substr($lastLine, $reasonSt, $reasonStp - $reasonSt);
      }
      elsif( $line =~ m/^MODM.*DEM.*RESTORE APPS PWR/ ) 
      {
        $appsState = 0;
        $appsStopTime = read_timestamp($line);

        #Store the values into my arrays.
        push (@aStart, $appsStartTime);
        push (@aStop, $appsStopTime);
        push (@aReason, $appsExitReason);
        push (@aSleep, $appsTimerSleepTime);
        push (@aAddr, $appsTimerAddress);
        push (@aSet, $appsTimerSet);
      }
      elsif( $line =~ m/^MODM.*ONCRPC.*CALL/ ) 
      {
        $appsExitReason = "ONCRPC: ";
        $reasonSt = index($line, "prog = ") + 7;
        $reasonStp = index($line, "  ", $reasonSt);
        $appsExitReason .= substr($line, $reasonSt, $reasonStp - $reasonSt);
      }
      $lastLine = $line;
    }
    elsif ($appsState==2)
    {
      # Determine the reason for waking up.  One of the below.
      #MODM:   58.770844    DEM: MAO INTS  00000001   00000000   00000000
      #MODM:   59.569250    DEM: PC TIMER EXPIRED  00000000   00000000   00000000
      #MODM:   61.392906    DEM: BATTERY INFO  00000002   00000000   00000000

      $reasonSt = index($line, "DEM: ") + 5;
      $reasonStp = index($line, "  ", $reasonSt);
      $appsExitReason = substr($line, $reasonSt, $reasonStp - $reasonSt);

      $appsState=1;
    }
  }

  # Print results for Apps Processor.
  printf ("\n\nResults for Apps Processor\n\n");

  if ((@aStart+0) == 0)
  {
    printf("No APPS PWRCLPS detected.\n");
  }
  else
  {
    # Calculate Statistics for Apps Processor.
    my @aCalc = calc_stats(\@aStart, \@aStop, \@aReason);

    displayOutput(\@aStart, \@aStop, \@aReason, \@aSleep, \@aAddr, \@aSet, \@aCalc);
  }

} # main
