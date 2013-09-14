#!/bin/sh

DISTNAME=$1

if [ -z ${DISTNAME} ]
then
    echo osx-ssh-build.sh is to be used by 'Make osxdist_ssh'
    exit 1
fi

if [ -z `which make` ]
then
    echo Xcode commandline tools missing!
    echo Xcode can be obtained from the App Store
    echo The tools can be installed from Xcode -> Preferences -> Downloads
    exit 1
fi

set -e

cd /tmp
tar xf ${DISTNAME}-source.tar.gz
cd ${DISTNAME}
make osxdist
