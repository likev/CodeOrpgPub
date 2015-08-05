#!/bin/sh

#====================================================================
#           startCh1RpgPmd.sh
#
# Starts the RPG Version of the Performance Data Display
#
# Arguments to the script:
#   "start" - Starts the controller and displays PMD, or requests PMD.
#   "stop"  - Closes the display.
#
# Arguments to the Java command line used to start the display:
#   1: Action code: 1 for normal display, 9 to close display.
#   2: Channel numer we are connecting to: 1 or 2
#   3: RDA hostname, the name of the RDA we communicate with.
#===================================================================
ARGS=1
EXIT_CODE=9
DEFAULT_CODE=0

# Change into $ORDA_HOME/bin directory, change this for RPG
cd $ORDA_HOME/bin

# If an argument was not passed in, 
if [ $# -ne "$ARGS" ]
then
    echo "Usage: startCh1RpgPmd.sh [start|stop]"
    exit 1
fi

if [ "$1" == "start" ]
then
    # Start the app normally
    nice java -DORDA_HOME=$ORDA_HOME -DLOG_CHAN=1 -server -Xms64M -Xmx128M -jar rpg_pmd.jar 1 1 rda1
else if [ "$1" == "stop" ]
then
    # Start the app with the EXIT CODE sent in
    nice java -DORDA_HOME=$ORDA_HOME -DLOG_CHAN=1 -server -Xms64M -Xmx128M -jar rpg_pmd.jar $EXIT_CODE 1 rda1
else
    echo "Usage: startCh1RpgPmd.sh [start|stop]"
    exit 1
fi
fi
