#!/bin/sh

# This ship rotation utility requires the Netpbm package and ppmtoxpm.
#
# Copy a 20x20 PPM containing a ship oriented straight up to the current
# directory.  Make sure it is named SHIPr.ppm (e.g. SCr.ppm, DDr.ppm).  Then:
#
# Usage:
#   rot.sh SHIP (e.g. SC, DD, CV, etc.  SB, WB, JS not recommended ;)
#
# It will generate a bunch of PPMs named SHIPrc-<rot-angle>.ppm,
# SHIP.ppm, and SHIP.xpm (the last two contain all the rotation angles).
#
# Normally, just copy SHIP.xpm over whatever ship you want to replace.
# If you want to see how the ship rotates first, use xv:
#   xv SHIPrc*.ppm
# then use the Next/Prev buttons to go back & forth between frames.
# Expect the ship rotation to be a little jittery - we're using integers,
# 20x20 pixmaps, and disabling antialiasing.
#
# The default rotation parameters below should be pretty good.  I
# generated the Ind CL, CV, DD, and FR from them.
#  
# this utility is not necessarily very user friendly 

SHIP=$1

cp ${SHIP}r.ppm ${SHIP}rc-000.ppm
pnmflip -tb ${SHIP}rc-000.ppm > ${SHIP}rc-180.ppm
pnmflip -r90 ${SHIP}rc-000.ppm > ${SHIP}rc-270.ppm
pnmflip -r270 ${SHIP}rc-000.ppm > ${SHIP}rc-090.ppm

pnmrotate -noantialias -22.5 ${SHIP}rc-000.ppm > ${SHIP}r-022.ppm
pnmrotate -noantialias -45 ${SHIP}rc-000.ppm > ${SHIP}r-045.ppm
pnmrotate -noantialias -67.5 ${SHIP}rc-000.ppm > ${SHIP}r-067.ppm

pnmrotate -noantialias -22.5 ${SHIP}rc-090.ppm > ${SHIP}r-112.ppm
pnmrotate -noantialias -45 ${SHIP}rc-090.ppm > ${SHIP}r-135.ppm
pnmrotate -noantialias -67.5 ${SHIP}rc-090.ppm > ${SHIP}r-157.ppm

pnmrotate -noantialias -22.5 ${SHIP}rc-180.ppm > ${SHIP}r-202.ppm
pnmrotate -noantialias -45 ${SHIP}rc-180.ppm > ${SHIP}r-225.ppm
pnmrotate -noantialias -67.5 ${SHIP}rc-180.ppm > ${SHIP}r-247.ppm

pnmrotate -noantialias -22.5 ${SHIP}rc-270.ppm > ${SHIP}r-292.ppm
pnmrotate -noantialias -45 ${SHIP}rc-270.ppm > ${SHIP}r-315.ppm
pnmrotate -noantialias -67.5 ${SHIP}rc-270.ppm > ${SHIP}r-337.ppm

# in quadrant 1, we want to shove the ship left & down for odd numbers
for i in 022 045 067; do
  pnmcrop ${SHIP}r-$i.ppm > /dev/null 2> $$.tmp
  pnmcrop ${SHIP}r-$i.ppm 2> /dev/null | head -2 >> $$.tmp
  tc=`grep "off the top" $$.tmp | awk '{print $3}'`
  if test -z "$tc"; then
    tc=0;
  fi
  lc=`grep "off the left" $$.tmp | awk '{print $3}'`
  if test -z "$lc"; then
    lc=0;
  fi;
  xdim=`tail -1 $$.tmp | awk '{print $1}'`
  ydim=`tail -1 $$.tmp | awk '{print $2}'`
  xcroppie=`expr 20 - $xdim`
  xcrop=`expr $xcroppie / 2`
  xcropodd=`expr $xcroppie % 2`
  ycroppie=`expr 20 - $ydim`
  ycrop=`expr $ycroppie / 2`
  ycropodd=`expr $ycroppie % 2`
  xo=`expr $lc - $xcrop`
  yo=`expr $tc - $ycrop - $ycropodd`
  pnmcut $xo $yo 20 20 ${SHIP}r-$i.ppm > ${SHIP}rc-$i.ppm
  echo ${SHIP}rc-$i.ppm
  echo Args: $tc $lc $xdim $ydim $xcroppie $xcrop $xcropodd $ycroppie $ycrop $ycropodd $xo $yo
done

# in quadrant 2, we want to shove the ship left & up for odd numbers
for i in 112 135 157; do
  pnmcrop ${SHIP}r-$i.ppm > /dev/null 2> $$.tmp
  pnmcrop ${SHIP}r-$i.ppm 2> /dev/null | head -2 >> $$.tmp
  tc=`grep "off the top" $$.tmp | awk '{print $3}'`
  if test "x$tc" = x; then
    tc=0;
  fi
  lc=`grep "off the left" $$.tmp | awk '{print $3}'`
  if test "x$lc" = x; then
    lc=0;
  fi
  xdim=`tail -1 $$.tmp | awk '{print $1}'`
  ydim=`tail -1 $$.tmp | awk '{print $2}'`
  xcroppie=`expr 20 - $xdim`
  xcrop=`expr $xcroppie / 2`
  xcropodd=`expr $xcroppie % 2`
  ycroppie=`expr 20 - $ydim`
  ycrop=`expr $ycroppie / 2`
  ycropodd=`expr $ycroppie % 2`
  xo=`expr $lc - $xcrop`
  yo=`expr $tc - $ycrop`
  pnmcut $xo $yo 20 20 ${SHIP}r-$i.ppm > ${SHIP}rc-$i.ppm
  echo ${SHIP}rc-$i.ppm
done

# in quadrant 3, we want to shove the ship right & up for odd numbers
for i in 202 225 247; do
  pnmcrop ${SHIP}r-$i.ppm > /dev/null 2> $$.tmp
  pnmcrop ${SHIP}r-$i.ppm 2> /dev/null | head -2 >> $$.tmp
  tc=`grep "off the top" $$.tmp | awk '{print $3}'`
  if test "x$tc" = x; then
    tc=0;
  fi
  lc=`grep "off the left" $$.tmp | awk '{print $3}'`
  if test "x$lc" = x; then
    lc=0;
  fi
  xdim=`tail -1 $$.tmp | awk '{print $1}'`
  ydim=`tail -1 $$.tmp | awk '{print $2}'`
  xcroppie=`expr 20 - $xdim`
  xcrop=`expr $xcroppie / 2`
  xcropodd=`expr $xcroppie % 2`
  ycroppie=`expr 20 - $ydim`
  ycrop=`expr $ycroppie / 2`
  ycropodd=`expr $ycroppie % 2`
  xo=`expr $lc - $xcrop - $xcropodd`
  yo=`expr $tc - $ycrop`
  pnmcut $xo $yo 20 20 ${SHIP}r-$i.ppm > ${SHIP}rc-$i.ppm
  echo ${SHIP}rc-$i.ppm
done

# in quadrant 4, we want to shove the ship right & down for odd numbers
for i in 292 315 337; do
  pnmcrop ${SHIP}r-$i.ppm > /dev/null 2> $$.tmp
  pnmcrop ${SHIP}r-$i.ppm 2> /dev/null | head -2 >> $$.tmp
  tc=`grep "off the top" $$.tmp | awk '{print $3}'`
  if test "x$tc" = x; then
    tc=0;
  fi
  lc=`grep "off the left" $$.tmp | awk '{print $3}'`
  if test "x$lc" = x; then
    lc=0;
  fi
  xdim=`tail -1 $$.tmp | awk '{print $1}'`
  ydim=`tail -1 $$.tmp | awk '{print $2}'`
  xcroppie=`expr 20 - $xdim`
  xcrop=`expr $xcroppie / 2`
  xcropodd=`expr $xcroppie % 2`
  ycroppie=`expr 20 - $ydim`
  ycrop=`expr $ycroppie / 2`
  ycropodd=`expr $ycroppie % 2`
  xo=`expr $lc - $xcrop - $xcropodd`
  yo=`expr $tc - $ycrop - $ycropodd`
  pnmcut $xo $yo 20 20 ${SHIP}r-$i.ppm > ${SHIP}rc-$i.ppm
  echo ${SHIP}rc-$i.ppm
done

pnmcat -tb ${SHIP}rc-*.ppm > ${SHIP}.ppm
ppmtoxpm ${SHIP}.ppm > ${SHIP}.xpm

rm -f $$.tmp
