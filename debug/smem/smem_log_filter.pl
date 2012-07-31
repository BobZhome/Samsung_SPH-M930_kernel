#!/usr/bin/perl -Xw

#****************************************************************************
#**
#**         smem_log_filter.pl
#**
#**  Filters smem log file to remove matching RPC calls and replies. Can
#**  operate on log file or standard input. Always writes to standard output.
#**
#** 
#** Copyright (c) 2006,2008 by QUALCOMM, Incorporated.  All Rights Reserved.
#****************************************************************************
#**
#**                        EDIT HISTORY FOR MODULE
#**
#**  $Header: //source/qcom/qct/core/pkg/2H09/halcyon_modem/main/latest/AMSS/products/7x30/tools/debug/smem_log_filter.pl#9 $
#**
#** when       who     what, where, why
#** --------   ---     ------------------------------------------------------
#** 03/20/08   sa      Added support for rpc router messages.
#** 08/26/06   ptm     Initial version.
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

  print STDERR "usage: smem_log_filter.pl [<smem_log_file>] \n";
  print STDERR "       Filters out matching RPC calls and replies. If the\n";
  print STDERR "       log file is not given, reads from standard input\n";
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
    while( <> ) {
      # delete end-of-line chars
      s/[\r\n]+//;
      push @lines, ($_);
    }
  }

  return \@lines;
} # read_log

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
  my $num_errors = 0;
  my $num_warnings = 0;
  my $replies;

  my $lines = read_log( $filename );
  my $num_lines = scalar @$lines;

  # Check each input line, if it's a reply add an entry to the replies hash
  # using the processor associated with the call and the xid as the key.
  foreach $line (@$lines) {
    if( $line =~ m/^APPS.*ONCRPC.*REPLY.*xid = ([0-9a-fA-F]+)/ ) {
      my $key = "MODM$1";
      $$replies{$key} = "no-match";
    }
    elsif( $line =~ m/^MODM.*ONCRPC.*REPLY.*xid = ([0-9a-fA-F]+)/ ) {
      my $key = "APPS$1";
      $$replies{$key} = "no-match";
    }
  }

  foreach $line (@$lines) {
    # if the line is a call, check the replies hash for a matching reply
    # and take the appropriate action.
    if( $line =~ m/^(....).*ONCRPC.*CALL.*xid = ([0-9a-fA-F]+)/ or
        $line =~ m/^(....).*ONCRPC.*ASYNC.*xid = ([0-9a-fA-F]+)/ ) {
      my $key = "$1$2";

      if( defined $$replies{$key} ) {
        # have a match do not print this line
        if( $$replies{$key} ne "no-match" ) {
          # already matched this reply - print error message
          $num_errors += 1;
          print STDOUT "\nERROR: multiple CALLS with same xid\n";
          print STDOUT "    $line\n\n";
          $$replies{$key} = "multi-match";
        }
        else {
          # first match for this reply - update replies hash
          $$replies{$key} = "matched";
        }
      }
      else {
        # no matching reply - print the call
        print "$line\n";
      }
    }
    # if the line is an APPS reply, check to see if it matched a call and
    # take the appropriate action.
    elsif( $line =~ m/^APPS.*ONCRPC.*REPLY.*xid = ([0-9a-fA-F]+)/ ) {
      my $key = "MODM$1";

      if( $$replies{$key} eq "no-match" ) {
        # No matching call - print warning message
        $num_warnings += 1;
        print STDOUT "\nWARNING: no matching CALL\n";
        print STDOUT "    $line\n\n";
      }
      elsif( $$replies{$key} eq "multi-match" ) {
        # multiple matching calls - print this line
        # (error already printed for call)
        print STDOUT "    $line\n";
      }
    }
    # if the line is a MODEM reply, check to see if it matched a call and
    # take the appropriate action.
    elsif( $line =~ m/^MODM.*ONCRPC.*REPLY.*xid = ([0-9a-fA-F]+)/ ) {
      my $key = "APPS$1";

      if( $$replies{$key} eq "no-match" ) {
        # No matching call - print warning message
        $num_warnings += 1;
        print STDOUT "\nWARNING: no matching CALL\n";
        print STDOUT "    $line\n\n";
      }
      elsif( $$replies{$key} eq "multi-match" ) {
        # multiple matching calls - print this line
        # (error already printed for call)
        print STDOUT "    $line\n";
      }
    }
    # if the line is a router event, take the appropriate action.
    elsif( $line =~ m/ROUTER:/ ) {
       # Ignore it for now
    }
    # if the line is a sleep event, take the appropriate action.
    elsif( $line =~ m/SLEEP:/ ) {
       # Ignore it for now
    }
    # neither a call or a reply, just print the line.
    else {
      print STDOUT "$line\n";
    }
  }

  # print number of warnings
  if( $num_warnings == 1 ) {
    print STDOUT "\n1 warning found\n";
  }
  elsif( $num_warnings > 1 ) {
    print STDOUT "\n$num_warnings warnings found\n";
  }

  # print number of errors
  if( $num_errors == 1 ) {
    print STDOUT "\n1 error found\n";
  }
  elsif( $num_errors > 1 ) {
    print STDOUT "\n$num_errors errors found\n";
  }
} # main
