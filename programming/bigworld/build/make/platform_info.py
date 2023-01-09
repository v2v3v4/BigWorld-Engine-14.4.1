#!/usr/bin/env python

# This file is used by the build system to determine the Linux distribution
# that the build is being performed on. The output is used to establish the
# correct output directories for files.
#
# Only supported BigWorld Technology distributions will generate output from
# this program. Unsupported platforms will exit with code of 1.
#
# Distribution     Expected output
# CentOS 5         el5
# CentOS 5.4       el5
# CentOS 6         el6
# CentOS 7         el7
# RHEL 5           el5
# RHEL 6.4         el6
# RHEL 7           el7
# Fedora 18        fedora18
#

#
# TODO:
# * When CentOS 5 support is dropped we can use platform.linux_distribution()
#   to save a lot of the grunt work here.
#

import platform
import re
import sys
import os

REDHAT_VERSION_FILE = "/etc/redhat-release"


# Examples:

# 'CentOS release 5.9 (Final)'
# 'CentOS release 6.4 (Final)'
# 
# 'Red Hat Enterprise Linux Server release 5.2 (Tikanga)'
# 'Red Hat Enterprise Linux Server release 6.3 (Santiago)'
# 
# 'Fedora release 17 (Beefy Miracle)'
# 
# CentOS 6+ have /etc/centos-release


# This is a convenience method for converting long distro names such as
# 'Red Hat Enterprise Linux Server' to 'rhel'
def longDistroNameToShort( longDistroName ):
	if longDistroName.startswith( "Red Hat" ):
		return "rhel"
	# On CentOS 7, longDistroName would be "CentOS Linux" rather then "CentOS".
	# Here we trim it to "CentOS"
	elif longDistroName.startswith( "CentOS" ):
		return "CentOS"

	return longDistroName


SHORT_NAME_ENTERPRISE_LINUX = "el"

ENTERPRISE_LINUX_DISTROS = [ "centos", "rhel" ]
ALLOWED_DISTROS = ENTERPRISE_LINUX_DISTROS + [ "fedora" ]

def finaliseShortNameFromReleaseInfo( longDistroName, versionStr, releaseName ):

	majorVerStr = versionStr

	# CentOS 7 returns something like 7.0.456
	if "." in versionStr:
		majorVerStr = versionStr[ 0 : versionStr.index(".") ]
		
	versionNum = int( majorVerStr )
	shortDistroName = longDistroNameToShort( longDistroName ).lower()

	if shortDistroName not in ALLOWED_DISTROS:
		sys.stderr.write( "Distribution '%s' is not supported\n" %
			shortDistroName )
		return None

	if shortDistroName in ENTERPRISE_LINUX_DISTROS:
		shortDistroName = SHORT_NAME_ENTERPRISE_LINUX

	return "%s%d" % (shortDistroName, versionNum)


# Regular expression taken from Python 2.6's platform.py
_lsb_release_version = re.compile(r'(.+)'
                                   ' release '
                                   '([\d.]+)'
                                   '[^(]*(?:\((.+)\))?')

def parseRedHatRelease():
	fp = open( REDHAT_VERSION_FILE, "r" )
	versionLine = fp.readline().strip()
	fp.close()

	# Note: we only support CentOS/RHEL (and for upgrade purposes Fedora) so
	# the LSB Release limitation should be fairly safe
	match = _lsb_release_version.match( versionLine )
	if match is not None:
		# LSB format: "distro release x.x (codename)"
		return finaliseShortNameFromReleaseInfo( *match.groups() )

	return None


def findPlatformName():
	"""This function parses and returns the platform name. It is separated from
	main() so that it can be used from pycommon/bwlog.py"""

	if not os.path.isfile( REDHAT_VERSION_FILE ):
		sys.stderr.write( "Unable to locate file: %s\n" % REDHAT_VERSION_FILE )
		return None

	releaseStr = parseRedHatRelease()
	if not releaseStr:
		sys.stderr.write( "Unable to parse file: %s\n" % REDHAT_VERSION_FILE )
		return None

	return releaseStr


def main():

	platformName = findPlatformName()

	if not platformName:
		return False

	print platformName

	return True


if __name__ == "__main__":
	try:
		if not main():
			print "unknown"
			sys.exit( 1 )

		sys.exit( 0 )
	except KeyboardInterrupt:
		sys.exit( 1 )


# platform_info.py
