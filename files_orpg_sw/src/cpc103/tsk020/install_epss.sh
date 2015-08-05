#!/bin/sh
#
# RCS info
# $Author: cmn $
# $Locker:  $
# $Date: 2008/11/03 21:55:32 $
# $Id: install_epss.sh,v 1.4 2008/11/03 21:55:32 cmn Exp $
# $Revision: 1.4 $
# $State: Exp $

#**************************************************************************
#
# Title:        install_epss
#
# Description:  Installs EPSS files
#
#**************************************************************************

CD_DIR=""
REMOVE_EPSS=NO
REMOVE_EPSS_ONLY=NO
INTERACTIVE_FLAG=YES
EPSS_MARKER_FILE=$HOME/.EPSS_is_installed
DISPLAY_IS_INSTALLED_FLAG=NO

#**************************************************************************
#**************************************************************************
#
# Start of Functions
#
#**************************************************************************
#**************************************************************************

#**************************************************************************
# PRINT_USAGE: Display "help" to screen.
#**************************************************************************

print_usage ()
{
  echo
  echo
  echo "************************************************"
  echo "This script installs EPSS"
  echo
  echo "The following are valid flags"
  echo
  echo "-h - help"
  echo "-c - print whether EPSS is already installed"
  echo "-r - remove previously installed EPSS files before installing"
  echo "-R - remove previously installed EPSS files and exit"
  echo "-N - non-interactive (invoked from script)"
  echo "************************************************"
  echo
  echo
}

#**************************************************************************
# ECHO_STDOUT: Echo to stdout.
#**************************************************************************

echo_stdout ()
{
  echo -e $1
  echo -e $1 | $LE_LOG_CMD
}

#**************************************************************************
# ECHO_STDERR: Echo to stderr.
#**************************************************************************

echo_stderr ()
{
  echo -e $1 >&2
  echo -e $1 | $LE_ERR_CMD
}

#**************************************************************************
# ECHO_INT: Echo if interactive. Output does not go to log file.
#**************************************************************************

echo_int ()
{
  if [ $INTERACTIVE_FLAG = "YES" ]
  then
    echo -e $1
  fi
}

#**************************************************************************
# ECHO_ERROR: Echo error message and exit.
#**************************************************************************

echo_error ()
{
  if [ $# -eq 1 ]
  then
    exit_code=1
  else
    exit_code=$2
  fi

  echo_int
  echo_int
  echo_int "IN SCRIPT: $0"
  echo_stderr "ERROR: $1"
  echo_int "Stopping script"
  echo_int "Use -h option for help"
  echo_int
  echo_int

  exit $exit_code
}

#**************************************************************************
# INIT_LE_LOG: Initialize logging.
#**************************************************************************

init_le_log ()
{
  LE_NUM_LINES=100

  LE_UID=`id -u 2>/dev/null`
  if [ $? -ne 0 -o "$LE_UID" = "" ]
  then
    echo_error "Command \"id -u 2>/dev/null\" failed ($?)"
  fi
  LE_GID=`id -g 2>/dev/null`
  if [ $? -ne 0 -o "$LE_GID" = "" ]
  then
    echo_error "Command \"id -g 2>/dev/null\" failed ($?)"
  fi
  LE_FILE_NAME=`basename $0 2>/dev/null`
  if [ $? -ne 0 -o "$LE_FILE_NAME" = "" ]
  then
    echo_error "Command \"basename $0 2>/dev/null\" failed ($?)"
  fi

  LE_LOG_CMD="le_pipe -t $LE_FILE_NAME -n $LE_NUM_LINES -w $LE_UID -g $LE_GID"
  LE_ERR_CMD="$LE_LOG_CMD -e GL_ERROR"
}

#**************************************************************************
# PARSE_INPUT: Parse command line input.
#**************************************************************************

parse_input ()
{
  # Parse input options/arguments. If option doesn't have an argument (and
  # requires one) or option isn't valid, print message and exit.

  while getopts hcrRN optionflag
  do
    case $optionflag in
       h) print_usage; exit 0;;
       c) DISPLAY_IS_INSTALLED_FLAG=YES;;
       r) REMOVE_EPSS=YES;;
       R) REMOVE_EPSS_ONLY=YES;;
       N) INTERACTIVE_FLAG=NO;;
      \?) echo_error "Invalid input flag"
    esac
  done

  # If user only wants to know if EPSS is installed, do it now. Put this
  # here (instead of calling in getopts loop) so all flags can be processed.

  if [ $DISPLAY_IS_INSTALLED_FLAG = "YES" ]
  then
    is_EPSS_installed
  fi
}

#**************************************************************************
# INITIAL_SETUP: Set initial variables.
#**************************************************************************

initial_setup ()
{
  # Make sure $EPSS_DIR is defined

  env | grep EPSS_DIR > /dev/null 2>&1
  if [ $? -ne 0 ]
  then
    echo_error "Environmental variable \$EPSS_DIR not defined"
  fi

  # Make sure EPSS_DIR directory exists

  mkdir -p $EPSS_DIR > /dev/null 2>&1
  if [ ! -d $EPSS_DIR ]
  then
    echo_error "Directory $EPSS_DIR does not exist"
  fi

  # If only removing EPSS, no need to continue

  if [ $REMOVE_EPSS_ONLY = "YES" ]
  then
    return 0
  fi

  # Mount CD
 
  echo_stdout "Mounting EPSS CD"

  CD_DIR=`medcp -pm cd`

  if [ $? -ne 0 ]
  then
    echo_error "Unable to mount CD" 2
  elif [ "$CD_DIR" = "" ]
  then
    echo_error "CD directory undefined"
  fi

  # Make sure this is an EPSS CD

  if [ ! -d $CD_DIR/epss ]
  then
    echo_error "$CD_DIR/epss does not exist"
  fi
}

#**************************************************************************
# IS_EPSS_INSTALLED: Determine if EPSS is installed.
#**************************************************************************

is_EPSS_installed ()
{
  # If file list exists, then assume EPSS is installed.
  
  if [ -e $EPSS_MARKER_FILE ]
  then
    echo_int
    echo_int
    echo_int "EPSS is installed."
    echo_int
    echo_int
    exit 1
  else
    echo_int
    echo_int
    echo_int "EPSS is not installed."
    echo_int
    echo_int
    exit 0
  fi
}

#**************************************************************************
# REMOVE_EPSS: Remove previously installed EPSS files.
#**************************************************************************

remove_EPSS ()
{
  # If not removing files, no need to continue

  if [ $REMOVE_EPSS = "NO" -a $REMOVE_EPSS_ONLY = "NO" ]
  then
    return 0
  fi

  # Make sure list of installed files is present

  if [ ! -f $EPSS_MARKER_FILE ]
  then
    if [ $REMOVE_EPSS_ONLY = "YES" ]
    then
      echo_int
      echo_int
      echo_stdout "No previous EPSS installation detected"
      echo_int
      echo_int
      exit 0
    else
      echo_stdout "No previous EPSS installation detected"
      return 0
    fi
  fi 
 
  echo_stdout "Removing previously installed EPSS files"

  rm -rf $EPSS_DIR 1>&2
  if [ $? -ne 0 ]
  then
    echo_error "Error removing files in $EPSS_DIR"
  fi
  rm -f $EPSS_MARKER_FILE 1>&2

  # If only removing files, print exit message.

  if [ $REMOVE_EPSS_ONLY = "YES" ]
  then
    echo_int
    echo_int
    echo_stdout "Successfully removed previously installed EPSS files"
    echo_int
    echo_int
    exit 0
  fi
}

#**************************************************************************
# INSTALL_EPSS: Install EPSS files.
#**************************************************************************

install_EPSS ()
{
  # Copy EPSS files over and create marker file

  cd $CD_DIR/epss

  echo_stdout "Copying files in $CD_DIR/epss to $EPSS_DIR"
  cp -r * $EPSS_DIR 1>&2
  if [ $? -ne 0 ]
  then
    rm -rf $EPSS_DIR
    echo_error "Error copying files from $CD_DIR to $EPSS_DIR"
  fi

  echo_stdout "Setting permissions of files in $EPSS_DIR"
  chmod -R 755 $EPSS_DIR 1>&2
  if [ $? -ne 0 ]
  then
    echo_error "Error setting permissions of files in $EPSS_DIR"
  fi

  echo_stdout "Creating $EPSS_MARKER_FILE"
  echo `date` > $EPSS_MARKER_FILE
  if [ $? -ne 0 ]
  then
    echo_error "Error creating $EPSS_MARKER_FILE" 8
  fi
}

#**************************************************************************
# CLEANUP_FUNCTION: Clean up loose ends.
#**************************************************************************

cleanup_function ()
{
  echo_stdout "Unmount and eject EPSS CD"
  # Change directory to make sure CD unmounts/ejects
  cd > /dev/null 2>&1
  medcp -ue cd 2> /dev/null
}

#**************************************************************************
#**************************************************************************
#
# End of Functions
#
#**************************************************************************
#**************************************************************************

#**************************************************************************
#**************************************************************************
#
# Main Body of Script
#
#**************************************************************************
#**************************************************************************

init_le_log
parse_input "$@"
initial_setup
remove_EPSS
install_EPSS
cleanup_function

echo_int
echo_int
echo_stdout "Successful installation of EPSS"
echo_int
echo_int

exit 0


