#!/bin/bash

# retreive from svn
svn export --force "https://svn.victronenergy.com/svn/vrm_database/trunk/dataAttributes en deviceTypes.xlsm"

#
# convert Data attributes to csv
#

# convert to csv
xlsx2csv -s 1 -d tab -i dataAttributes\ en\ deviceTypes.xlsm attributes_export.csv
# remove first 3 lines
sed -i '1,3d' attributes_export.csv
# replace comma's with dash -
sed -i 's/,/-/g' attributes_export.csv
# get meaningfull columns
cut -f5,6,7,14,15,16 attributes_export.csv > attributes.csv
# cleanup empty lines
sed -i '/^\t/d' attributes.csv
# replace tabs with comma's
sed -i 's/\t/,/g' attributes.csv

# convert to csv
xlsx2csv -s 3 -i dataAttributes\ en\ deviceTypes.xlsm unitid2di.csv
