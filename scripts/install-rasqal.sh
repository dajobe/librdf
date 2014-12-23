#!/bin/sh
# Helper script to install a particular rasqal version
#
# Intended for use with travis CI; see .travis.yml at the root

set -x

PACKAGE=rasqal
MIN_VERSION=$1
INSTALL_VERSION=$2

FILE="$PACKAGE-$INSTALL_VERSION.tar.gz"
URL="http://download.librdf.org/source/$FILE"

ROOT_DIR=${TMPDIR:-/tmp}
BUILD_DIR="$ROOT_DIR/build-$PACKAGE"

rasqal_version=`pkg-config --modversion $PACKAGE`
if pkg-config --atleast-version $MIN_VERSION $PACKAGE; then
    echo "Rasqal $rasqal_version is new enough"
else
    mkdir $BUILD_DIR && cd $BUILD_DIR
    wget -O $FILE $URL || curl -o $FILE $URL
    tar -x -z -f $FILE && rm $FILE
    cd $PACKAGE-$INSTALL_VERSION && ./configure --prefix=/usr && make && sudo make install
    cd / && rm -rf $BUILD_DIR
fi
