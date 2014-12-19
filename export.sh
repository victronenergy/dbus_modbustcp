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
cut -f5,6,7,8,15,16,17,21 attributes_export.csv > attributes.csv
# cleanup empty lines
sed -i '/^\t/d' attributes.csv
# replace tabs with comma's
sed -i 's/\t/,/g' attributes.csv
# replace operator or owner (accessLevel)  with W for write permissions
out="$(mktemp)"
awk 'BEGIN{FS=OFS=","} { if ($8 =="owner" || $8=="operator" ) gsub($8, "W"); else $8="R"; }'1 attributes.csv > $out
mv $out attributes.csv
# remove lines without modbus registerid
sed -i '/,,,,W$/d' attributes.csv
sed -i '/,,,,R$/d' attributes.csv

# convert to csv
xlsx2csv -s 3 -i dataAttributes\ en\ deviceTypes.xlsm unitid2di.csv

# cleanup
rm attributes_export.csv
