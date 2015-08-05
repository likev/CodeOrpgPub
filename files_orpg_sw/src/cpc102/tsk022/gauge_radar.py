#!/usr/bin/python

# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2011/05/09 19:59:28 $
# $Id: gauge_radar.py,v 1.4 2011/05/09 19:59:28 steves Exp $
# $Revision: 1.4 $
# $State: Exp $


import sys, urllib2,os,cgi,time, glob

import numpy as np

# note change in "bfiedler" needed below, task defined at end

#########################################

import matplotlib  #just for the purpose of the following statement:

matplotlib.use('Agg') #prevents matplotlib from preparing to open a GUI

###########################################################

from pylab import *

import matplotlib.numerix.ma as M

from matplotlib.dates import YearLocator, MonthLocator, DayLocator, DateFormatter, HourLocator

import datetime

# jfw import pdb

####################################

mesonet_file="http://www.mesonet.org/data/public/mesonet/mdf/YYYY/MM/DD/YYYYMMDDTTTT.mdf"

#######################################################

# jfw software switch for selecting 1 of the 4 rain gauge networks
# Set a switch to 0 to not query that network

use_mesonet  = 1
use_ltwash   = 1
use_ftcobb   = 1
use_okcmicro = 1

# process parameters passed to this script:
# jfw - send it command line arguments
# jfw - removed all prints to make it silent
# jfw - made outdir 1st arguement

# print "Gathering Precipitation Data..."

outdir = sys.argv[1]

# print "Start date (yyyymmdd)?"
# start_yyyymmdd = raw_input()

start_yyyymmdd = sys.argv[2]

# print "Start time (hhmm)?"
# start_time = raw_input()

start_time = sys.argv[3]

# print "End date (yyyymmdd)?"
# end_yyyymmdd = raw_input()

end_yyyymmdd = sys.argv[4]

# print "End time (hhmm)?"
# end_time = raw_input()

end_time = sys.argv[5]

#try:

#	start_yyyymmdd = "20090903"

#	start_time = "0000"

#	end_yyyymmdd = "20090903"

#	end_time = "2000"
#except:

#	yyyymmdd="20051008" #default date

#	start_time="0000"
#	end_time = "2355"


#####################################################

# construct mesonet file name, and retrieve file

if start_time[3] != "0":

	if start_time[3] != "5":

		# print "Invalid start time!"

		sys.exit()

if end_time[3] != "0":

	if end_time[3] != "5":

		# print "Invalid end time!"

		sys.exit()


yyyy=start_yyyymmdd[0:4]

mm=start_yyyymmdd[4:6]

dd=start_yyyymmdd[6:8]

f_yyyy=end_yyyymmdd[0:4]

f_mm=end_yyyymmdd[4:6]

f_dd=end_yyyymmdd[6:8]



startingdate = datetime.date(int(yyyy),int(mm),int(dd))

endingdate = datetime.date(int(f_yyyy),int(f_mm),int(f_dd))

start_dnum = date2num(startingdate)

end_dnum = date2num(endingdate)



days_between  = int(end_dnum - start_dnum)

#print "Days Between: ", days_between


dates=[]


current_date = num2date(start_dnum)

current_dnum = start_dnum
#print "Current date: ", current_date



def read_infile(filetype, yyyy, mm, dd, end_time):

	if filetype == "meso":

		file="http://www.mesonet.org/data/public/mesonet/mdf/YYYY/MM/DD/YYYYMMDDTTTT.mdf"

	elif filetype == "ltwash":

		file="http://www.mesonet.org/data/public/ars/mdf/YYYY/MM/DD/YYYYMMDDTTTT.mdf"

	elif filetype == "ftcobb":

		file="http://www.mesonet.org/data/public/fcars/mdf/YYYY/MM/DD/YYYYMMDDTTTT.mdf"

	elif filetype == "okcmicro":

		file="http://www.mesonet.org/data/public/okc/mdf/YYYY/MM/DD/YYYYMMDDTTTT.mdf"

	else:

		print "Unrecognized file type!"		

	file=file.replace('YYYY',yyyy)
	file=file.replace('MM',mm)

	file=file.replace('DD',dd)
	file=file.replace('TTTT',end_time)

	#print 'getting file: ',file

	rains=[]
	stids=[]

	try:

		content=urllib2.urlopen(file).readlines() # this reads the text at the URL "file"

	except:

		print "Cannot open ", file

                # On 6/14/2010 22 Z, the ARS Mesonet went down due to a lightning strike.
                # We now exit, returning no gauges, but instead we should continue on
                # and get the working gauges. For future enhancement ...

		sys.exit()

                # For future enhancement ...
                # return stids, rains

	##############################################

	# process the data in the file

	nl=0

	nummissing=0

	for line in content:

                # jfw debug: print line

		nl+=1

		if nl==1 : continue #copyright line, ignore

		if nl==2 : # time header


			try:

				line=line.strip()

				year,month,day=line.split()[1:4]

				yr=int(year)

				mn=int(month)

				dy=int(day)

			except:

				# print "choked on parsing time time info"

				sys.exit()

			continue

		if nl==3 : continue # column labels, ignore

		line=line.strip()

		items=line.split()

		try:

			if filetype == "meso":

				stid,stnm,minutes,relh,tair,wspd,wvec,wdir,wdsd,wssd,wmax,rain,pres=items[0:13]

			elif filetype == "okcmicro":

				stid,stnm,time,tair,relh,pres,wspd,wvec,wdir,wdsd,wssd,wmax,rain=items[0:13]

			else:

				stid,stnm,time,rain=items[0:4]

		except:

			# print "It crashed! The line did not split into items correctly!"

			sys.exit()

		

		stids.append(stid)

		rainfloat=float(rain)

		if rainfloat < -990.:

			nummissing+=1

		# jfw DON"T Convert to inches - loses precision

		# rainfloat = rainfloat * 0.03937007

		rains.append(rainfloat)

	if end_time == "0000":

		for n in range(len(rains)):

			if rains[n] != 0.0:

				rains[n] = 0.0

			


	# print "number of missing values =",nummissing

	return stids, rains


for n in range(days_between + 1):

	# print n

	if n == 0:

		beginning = start_time

	else:

		beginning = "0000"

	if n == days_between:

		ending = end_time

	else:

		ending = "2355"

	year = str(num2date(current_dnum))[0:4]

	month = str(num2date(current_dnum))[5:7]

	day = str(num2date(current_dnum))[8:10]


        if use_mesonet == 1:

	   start_mesonet_precips = read_infile("meso",year,month,day,beginning)[1]

	   end_mesonet_precips = read_infile("meso",year,month,day,ending)[1]

        if use_ltwash == 1:

    	   start_ltwash_precips = read_infile("ltwash",year,month,day,beginning)[1]

	   end_ltwash_precips = read_infile("ltwash",year,month,day,ending)[1]

        if use_ftcobb == 1:

	   start_ftcobb_precips = read_infile("ftcobb",year,month,day,beginning)[1]

	   end_ftcobb_precips = read_infile("ftcobb",year,month,day,ending)[1]

        if use_okcmicro == 1:

	   start_okcmicro_precips = read_infile("okcmicro",year,month,day,beginning)[1]

	   end_okcmicro_precips = read_infile("okcmicro",year,month,day,ending)[1]

	if n == 0:

          if use_mesonet == 1:

		mesonet_precips = np.subtract(end_mesonet_precips, start_mesonet_precips)	

		mesonet_ids = read_infile("meso",year,month,day,beginning)[0]

          if use_ltwash == 1:

		ltwash_precips = np.subtract(end_ltwash_precips, start_ltwash_precips)

		ltwash_ids = read_infile("ltwash",year,month,day,beginning)[0]

          if use_ftcobb == 1:

		ftcobb_precips = np.subtract(end_ftcobb_precips, start_ftcobb_precips)

		ftcobb_ids = read_infile("ftcobb",year,month,day,beginning)[0]

          if use_okcmicro == 1:

		okcmicro_precips = np.subtract(end_okcmicro_precips, start_okcmicro_precips)

		okcmicro_ids = read_infile("okcmicro",year,month,day,beginning)[0]

	else:

          if use_mesonet == 1:

		mesonet_precips = np.add(mesonet_precips, np.subtract(end_mesonet_precips, start_mesonet_precips))

          if use_ltwash == 1:

		ltwash_precips = np.add(ltwash_precips, np.subtract(end_ltwash_precips, start_ltwash_precips))

          if use_ftcobb == 1:

		ftcobb_precips = np.add(ftcobb_precips, np.subtract(end_ftcobb_precips, start_ftcobb_precips))

          if use_okcmicro == 1:

		okcmicro_precips = np.add(okcmicro_precips, np.subtract(end_okcmicro_precips, start_okcmicro_precips))

	current_dnum = current_dnum + 1

# for n in range(len(mesonet_ids)):

	# print mesonet_ids[n], "%5.2f" % mesonet_precips[n]

# for n in range(len(ltwash_ids)):

	# print ltwash_ids[n], "%5.2f" % ltwash_precips[n]

# for n in range(len(ftcobb_ids)):

	# print ftcobb_ids[n], "%5.2f" % ftcobb_precips[n]

# for n in range(len(okcmicro_ids)):

	# print okcmicro_ids[n], "%5.2f" % okcmicro_precips[n]

# jfw pdb.set_trace()

# jfw added outdir

outfile = open('%s/%s%s_to_%s%s_precips.csv' % \
              (outdir, start_yyyymmdd, start_time, end_yyyymmdd, end_time), 'w')

# jfw removed some output lines

# print >>outfile, '%s, %s' % (start_yyyymmdd, end_yyyymmdd)

# print >>outfile, '%s, %s' % (start_time, end_time)

# print >>outfile, 'SiteID, Precip(in.)'

# jfw added format for mesonet_ids for easier pickup on other side.

if use_mesonet == 1:

   for n in range(len(mesonet_ids)):

	# print >>outfile, mesonet_ids[n], ", %5.2f" % mesonet_precips[n]

	print >>outfile, "%8s" % mesonet_ids[n], ", %5.2f" % mesonet_precips[n]

if use_ltwash == 1:

   for n in range(len(ltwash_ids)):

	# print >>outfile, ltwash_ids[n], ", %5.2f" % ltwash_precips[n]

	print >>outfile, "%8s" % ltwash_ids[n], ", %5.2f" % ltwash_precips[n]

if use_ftcobb == 1:

   for n in range(len(ftcobb_ids)):

   	# print >>outfile, ftcobb_ids[n], ", %5.2f" % ftcobb_precips[n]

	print >>outfile, "%8s" % ftcobb_ids[n], ", %5.2f" % ftcobb_precips[n]

if use_okcmicro == 1:

   for n in range(len(okcmicro_ids)):

	# print >>outfile, okcmicro_ids[n], ", %5.2f" % okcmicro_precips[n]

	print >>outfile, "%8s" % okcmicro_ids[n], ", %5.2f" % okcmicro_precips[n]

outfile.close()
