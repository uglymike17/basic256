#!/bin/bash
# ziplinuxsource.sh
# create a zip file of the trunk source for uploading to sourceforge
# rewrote to better align with the creation of debian build folders 2014-01-23 j.m.reneau

# 2020-04-28 j.m.reneau updated for 2.0.0.1


VERSION=`cat Version.h | grep "#define VERSION " | sed -e 's/#define VERSION "\(.*\)/\1/;s/ .*//'`

# clean up binary and previous zips of this version
make clean
rm -Rf ../basic256-$VERSION
mkdir ../basic256-$VERSION
rm ../basic256_$VERSION.orig.tar
rm ../basic256_$VERSION.orig.tar.gz

# copy the entire "trunk"
cp -r * ../basic256-$VERSION

# delete files that are not needed
rm ../basic256-$VERSION/*.bat
rm ../basic256-$VERSION/*.bin
rm ../basic256-$VERSION/*.exe
rm ../basic256-$VERSION/*.kbs
rm ../basic256-$VERSION/*.pdf
rm ../basic256-$VERSION/*.sqlite3
rm ../basic256-$VERSION/BASIC256Android.pro
rm ../basic256-$VERSION/BASIC256Portable.pro
rm ../basic256-$VERSION/Makefile*
rm ../basic256-$VERSION/basic256.sln
rm ../basic256-$VERSION/basic256.vcproj
rm ../basic256-$VERSION/object_script.*
rm ../basic256-$VERSION/ziplinuxsource.sh
find ../basic256-$VERSION -type f -name "*~" -exec rm -f {} \;

# delete svn and other directories that may exist
rm -R ../basic256-$VERSION/*~
rm -R ../basic256-$VERSION/.*
rm -R ../basic256-$VERSION/BASIC256
rm -R ../basic256-$VERSION/BASIC256Portable
rm -R ../basic256-$VERSION/BASIC256PortableDebug
rm -R ../basic256-$VERSION/android
rm -R ../basic256-$VERSION/basic256
rm -R ../basic256-$VERSION/debian
rm -R ../basic256-$VERSION/debug
rm -R ../basic256-$VERSION/release
rm -R ../basic256-$VERSION/tmp
rm -R ../basic256-$VERSION/tmp_portable_debug
rm -R ../basic256-$VERSION/tmp_portable_release
rm -R ../basic256-$VERSION/utility_maintenance_programs
rm -R ../basic256-$VERSION/wikihelp/wiki

# tar and zip
tar -cvf ../basic256_$VERSION.orig.tar ../basic256-$VERSION
gzip ../basic256_$VERSION.orig.tar

# cleanup
rm -Rf ../basic256-$VERSION
