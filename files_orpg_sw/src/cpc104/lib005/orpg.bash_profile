# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/06/27 20:08:34 $
# $Id: orpg.bash_profile,v 1.46 2013/06/27 20:08:34 steves Exp $
# $Revision: 1.46 $
# $State: Exp $
#
# File: orpg.profile 
#
# This is a template to use for the RPG .profile file.
# When logging in under the Bash shell, the .profile file is
# automatically sourced by the shell.
#

# Source global definitions
if [ -f /etc/bashrc ]; then
  . /etc/profile
  . /etc/bashrc
fi

# Standard location of the configuration files.
export CFG_DIR=$HOME/cfg

# Standard location of the EPSS files.
export EPSS_DIR=$HOME/epss

# Standard location of the product data. It is specified # this way to allow product data to be maintained on a
# hard disk different than the one used for programs.
export ORPGDIR=$HOME/data

# Standard location of task-private files and/or data stores.
export WORK_DIR=$HOME/tmp

# Standard location of ORDA-related files.
export ORDA_HOME=$HOME/orda

# The following variable is the port number used by the remote HCI.
export RMTPORT=50000

# The following variable sets environment for ldmadmin.
export LDMHOME=/opt/roc_ldm

# Location of LogError LB files. Event code is event
# to be posted upon receipt of critical LE messages.
# IMPORTANT: the event code should match ORPGEVT_LE_CRITICAL in orpgevt.h!!!
export LE_DIR_EVENT=$ORPGDIR"/logs:18"

# Set misc system variables.
umask 002            # Default umask.
PS1='\h ~\u:${PWD}>' # Primary prompt that we want.
PS2='\h ~\u:${PWD}>' # Secondary prompt that we want.
HISTSIZE=64          # Number of previous commands to remember.
TMOUT=0              # Amount of time to wait for input.
alias h='fc -l -50'  # Alias 'history' to 'h'.
alias which='type -a'  # Alias 'history' to 'h'.
unset SESSION_MANAGER DBUS_SESSION_BUS_ADDRESS # Allow ~rpg to run Gnome apps

# Variables for development tools

# XPDT
export ORPG_PRODUCTS_DATABASE=$ORPGDIR/pdist/product_data_base.lb
export XPDT_MAP_FILE=$HOME/data/ktlx.map
# CVG
export CVG_DEF_PREF_DIR=$HOME


