#!/bin/bash

# retreive from svn
svn export --force "https://svn.victronenergy.com/svn/vrm_database/trunk/dataAttributes en deviceTypes.xlsm"

#
# convert Data attributes to csv
#

# convert to csv
xlsx2csv -s 1 -i dataAttributes\ en\ deviceTypes.xlsm attributes_export.csv
# remove first 3 lines
sed -i '1,3d' attributes_export.csv
# get column 5 and 6
cut -d "," -f5,6,13,14,15 attributes_export.csv > attributes.csv
# cleanup empty lines
sed -i '/^,/d' attributes.csv

# convert to csv
xlsx2csv -s 3 -i dataAttributes\ en\ deviceTypes.xlsm unitid2di.csv
