#====================================================================
#           startCh1MSCF.sh
#
# This scripts will run the Channel 1 RDA HCI from the MSCF
#
#
#====================================================================
# send a kill -15(SIGTERM) to any existing running RDA HCI
# Find all instances of the rda_hci.jar running
PROC_IDS=$( ps -elf | grep rda_hci.jar | grep rda1 | grep -v grep | awk '{print $4}' )
#If we found any
if [ "$PROC_IDS" ]
then
    echo "Sending SIGTERM to RDA HCI: " $PROC_IDS
    # send a kill -15 (SIGTERM)
    kill -15 $PROC_IDS
fi

sleep 1

# If any HCI did not respond to the SIGTERM, they are REALLY hung
PROC_IDS=$( ps -elf | grep rda_hci.jar | grep rda1 | grep -v grep | awk '{print $4}' )
#If we found any
if [ "$PROC_IDS" ]
then
    echo "Sending SIGKILL to RDA HCI: " $PROC_IDS
    # send a kill -9 (SIGKILL)
    kill -9 $PROC_IDS
fi

# Change into the $ORDA_HOME/bin directory
cd $ORDA_HOME/bin

# Start RDA HCI
java -DORDA_HOME=$ORDA_HOME -DLOG_CHAN=1 -DServerIP=rda1 -server -jar rda_hci.jar 
