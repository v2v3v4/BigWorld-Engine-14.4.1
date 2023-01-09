# This file is used by the build system to get the svn revision number

import subprocess


def loadRevisionInfo():
	# Extract the REV from local copy.
	cmd = [ "svnversion", "-n" ]

	try:
		pipe = subprocess.Popen( cmd,
				stdout = subprocess.PIPE, stderr = subprocess.STDOUT )
	except OSError:
		print "Failed to execute '%s'" % " ".join( cmd )
		return 0

	try:
		revision = pipe.stdout.read()
		if revision == "Unversioned directory":
			return 0
	except:
		return 0

	return int( revision )
# loadRevisionInfo
