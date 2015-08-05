#!/bin/sh

# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/19 18:55:17 $
# $Id: update_virus_definition_files.sh,v 1.3 2010/05/19 18:55:17 ccalvert Exp $
# $Revision: 1.3 $
# $State: Exp $

#**************************************************************************
#
# Title:	update_virus_definition_files
#
# Description:	Updates antivirus software with latest virus
#               definition file (VDF)
#
#**************************************************************************

INTERACTIVE_FLAG=YES
CD_DIR=""

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
  echo "This script updates the system's antivirus software"
  echo "with the latest virus definition file."
  echo
  echo "The following are valid flags."
  echo
  echo "-h - help"
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

  LE_UID=`id -u rpg 2>/dev/null`
  ret=$?
  if [ $ret -ne 0 -o "$LE_UID" = "" ]
  then
    echo_error "Command \"id -u rpg 2>/dev/null\" failed ($ret)"
  fi
  LE_GID=`id -g rpg 2>/dev/null`
  ret=$?
  if [ $ret -ne 0 -o "$LE_GID" = "" ]
  then
    echo_error "Command \"id -g rpg 2>/dev/null\" failed ($ret)"
  fi
  LE_FILE_NAME=`basename $0 2>/dev/null`
  ret=$?
  if [ $ret -ne 0 -o "$LE_FILE_NAME" = "" ]
  then
    echo_error "Command \"basename $0 2>/dev/null\" failed ($ret)"
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

  while getopts hN optionflag
  do
    case $optionflag in
       h) print_usage; exit 0;;
       N) INTERACTIVE_FLAG=NO;;
      \?) echo_error "Invalid input flag."
    esac
  done
}

#**************************************************************************
# CURRENT_VDF_VERSION: Version of currently installed VDF.
#**************************************************************************

current_vdf_version ()
{
  CURRENT_VDF_FILE_VERSION=`uvscan --version 2>/dev/null | egrep "^Dat set version:" | awk '{print $4}'`

  if [ "$CURRENT_VDF_FILE_VERSION" = "" ]
  then
    CURRENT_VDF_FILE_VERSION=UNKNOWN
  fi
}

#**************************************************************************
# INITIAL_SETUP: Set initial variables.
#**************************************************************************

initial_setup ()
{
  # Make sure user has root priviliges

  if [ `whoami` != "root" ]
  then
    echo_error "You do not have root priviliges"
  fi

  # Mount CD

  echo_stdout "Mounting RPG Virus Scan Update CD"

  CD_DIR=`medcp -pm cd`
  ret=$?  

  if [ $ret -ne 0 ]
  then
    echo_error "Unable to mount CD ($ret)" $ret
  elif [ "$CD_DIR" = "" ]
  then
    echo_error "CD directory undefined"
  fi
  echo_stdout "CD mount: $CD_DIR"

  # Make sure update file is on CD.

  UPDATED_VDF_FILE=`ls $CD_DIR | egrep '^uvscan-dat-[0-9]+.*\.rpm$'`

  if [ "$UPDATED_VDF_FILE" = "" ]
  then
    echo_error "No virus definition update rpm found on $CD_DIR."
  fi

  echo_stdout "VDF RPM: $UPDATED_VDF_FILE"

  # Get version of the new VDF file.

  UPDATED_VDF_FILE_VERSION=`echo $UPDATED_VDF_FILE | awk -F- '{print $3}'`

  if [ "$UPDATED_VDF_FILE_VERSION" = "" ]
  then
    echo_error "Unable to obtain version of updated virus definition file"
  fi

  current_vdf_version

  echo_stdout "Current VDF version: $CURRENT_VDF_FILE_VERSION"
  echo_stdout "Updated VDF version: $UPDATED_VDF_FILE_VERSION"

  if [ $CURRENT_VDF_FILE_VERSION = "UNKNOWN" ]
  then
    echo_stdout "Installing virus definition file rpm"
  elif [ $UPDATED_VDF_FILE_VERSION -eq $CURRENT_VDF_FILE_VERSION ]
  then
    echo_stdout "Virus definition file is already up to date"
    exit 0
  elif [ $UPDATED_VDF_FILE_VERSION -lt $CURRENT_VDF_FILE_VERSION ]
  then
    echo_stdout "Current virus definition file is newer than updated file"
    echo_stdout "Exit without updating"
    exit 0
  else
    echo_stdout "Updating virus definition file"
  fi
}

#**************************************************************************
# UPDATE_VDF: Updates VDFs from updated file on CD.
#**************************************************************************

update_vdf ()
{
  # Install rpm.

  rpm -Uvh $CD_DIR/$UPDATED_VDF_FILE > /dev/null 2>&1
  ret=$?

  if [ $ret -ne 0 ]
  then
    echo_error "Error installing $UPDATED_VDF_FILE" $ret
  fi
}

#**************************************************************************
# VERIFY_UPDATE: Verify that VDFs were updated.
#**************************************************************************

verify_update ()
{
  # Check version of VDF file returned from antivirus software against
  # the version of the updated file.

  current_vdf_version

  if [ "$CURRENT_VDF_FILE_VERSION" = "UNKNOWN" ]
  then
    echo_error "Version of virus definition file is unknown. Update failed."
  elif [ $CURRENT_VDF_FILE_VERSION -ne $UPDATED_VDF_FILE_VERSION ]
  then
    echo_error "Version of VDF file ($CURRENT_VDF_FILE_VERSION) does not match updated VDF file ($UPDATED_VDF_FILE_VERSION)."
  fi
}

#**************************************************************************
# CLEANUP_FUNCTION: Clean up loose ends.
#**************************************************************************

cleanup_function ()
{
  echo_stdout "Unmount and eject RPG Virus Scan Update CD"

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
update_vdf
verify_update
cleanup_function

echo_int
echo_int
echo_stdout "Successful update of virus definition file with $UPDATED_VDF_FILE."
echo_int
echo_int

exit 0

