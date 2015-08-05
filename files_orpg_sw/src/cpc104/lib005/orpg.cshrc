# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/06/27 20:08:34 $
# $Id: orpg.cshrc,v 1.53 2013/06/27 20:08:34 steves Exp $
# $Revision: 1.53 $
# $State: Exp $
#
# File: orpg.cshrc
#
# This is a template to use for the RPG .cshrc file.
# When logging in under the C shell, the .cshrc file is
# automatically sourced by the shell.
#

# Standard location of the configuration files.
setenv CFG_DIR $HOME/cfg

# Standard location of the EPSS files.
setenv EPSS_DIR $HOME/epss

# Standard location of the product data. It is specified
# this way to allow product data to be maintained on a
# hard disk different than the one used for programs.
setenv ORPGDIR $HOME/data

# Standard location of ORDA-related files.
setenv ORDA_HOME $HOME/orda

# Standard location of task-private files and/or data stores.
setenv WORK_DIR $HOME/tmp

# The following variable is the port number used by the remote HCI.
setenv RMTPORT 50000

# The following variable sets environment for ldmadmin.
setenv LDMHOME /opt/roc_ldm

# Location of LogError LB files. Event code is event
# to be posted upon receipt of critical LE messages.
# IMPORTANT: the event code should match ORPGEVT_LE_CRITICAL in orpgevt.h!!!
setenv LE_DIR_EVENT $ORPGDIR"/logs:18"

# Set misc system variables.
umask 002                          # Default umask.
limit descriptors 256              # Max number of file descriptors.
if ( $?prompt ) then               # If this is an interactive shell...
    alias setprompt 'set prompt = "`hostname`:`pwd`>"' # Prompt that we want.
    setprompt                      # Set prompt initially.
    alias cd 'cd \!*;setprompt'    # Make sure prompt is reset after 'cd'.
    set history=64                 # Number of previous commands to remember.
    set savehist=64                # Number to save across sessions.
    alias h history                # Alias 'history' to 'h'.
endif


# Variables for development tools.

# XPDT
setenv ORPG_PRODUCTS_DATABASE $ORPGDIR/pdist/product_data_base.lb
setenv XPDT_MAP_FILE $HOME/data/ktlx.map
# CVG
setenv CVG_DEF_PREF_DIR $HOME


