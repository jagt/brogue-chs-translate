#!/usr/bin/env bash

#  Check whether a debian style package is currently installed.  
#  If we can't find dpkg, then we'll assume a non-Debian system and 
#  ignore the check.
function check_package
{
	if [ -z `which dpkg` ]
	then
		return 0
	fi

	if ! dpkg -s $1 >/dev/null 2>&1
	then
		echo Missing $1: use \"sudo apt-get install $1\"
		return 1
	fi

	return 0
}

#  Check a list of packages, printing messages for any missing packages
#  and returning success only if all are present.
function check_all
{
	any_missing=0

	for package in $*
	do
		if ! check_package $package
		then
			any_missing=1
		fi
	done
	
	return $any_missing
}

#check_all $*
