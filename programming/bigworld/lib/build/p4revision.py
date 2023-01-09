# This file is used by the build system to get the perforce revision number

import re
import logging
import subprocess
import os

logging.basicConfig()
log = logging.getLogger( __name__ )


class AuthenticationException( Exception ):
	""" Indicates that the credentials supplied by the user are invalid for the
		requested resource or method """
	pass


def loadRevisionInfo():
	localRev = ""
	client = ""

	command = ["p4", "info"]
	cmdOutput = runConnectedP4Command( command )
	if not cmdOutput:
		return None

	clientUnknown = re.findall( "Client unknown.", cmdOutput )

	if clientUnknown:
		# Not a perforce environment
		return None

	clientSearch = re.search( "Client name: (\S+)", cmdOutput )
	try:
		client = clientSearch.groups()[0]
	except:
		# p4 not behaving normally.
		log.warning( "Unable to get client name from 'p4 info'." )
		return None

	# Ensure we are within a workspace (this command will raise exceptions
	# if we are not in a workspace directory).
	command = ["p4", "where" ]
	cmdOutput = runConnectedP4Command( command )
	if not cmdOutput:
		return None

	# get the changelist info from 'p4 changes'
	localRev = getP4Changes( clientLabel="@" + client )

	return int( localRev )
# loadRevisionInfo


def getP4Changes( clientLabel = "" ):

	command = ["p4", "changes", "-s", "submitted", "-m1" ]
	if clientLabel:
		command.append( clientLabel )

	p4Proc = subprocess.Popen( command,
		stdout = subprocess.PIPE, stderr = subprocess.PIPE )
	out, err = p4Proc.communicate()

	cmdOutput = runConnectedP4Command( command )
	if not cmdOutput:
		return None

	# Changelist ID search pattern
	regSearch = re.compile( "^Change (\S+)", re.MULTILINE )

	changeLists = regSearch.search( cmdOutput )
	if not changeLists:
		log.warning( "No changelists found." )
		return None

	return changeLists.groups()[0]
# getP4Changes


def runConnectedP4Command( command ):

	p4Ticket = os.getenv( "P4TICKET" )

	if p4Ticket:
		command.insert( 1, "-P" )
		command.insert( 2, str( p4Ticket ) )

	p4Proc = subprocess.Popen( command,
		stdout = subprocess.PIPE, stderr = subprocess.PIPE )
	out, err = p4Proc.communicate()

	if p4Proc.returncode :
		errMessage = "Unable to get revision information: " + \
			err.rstrip( os.linesep )

		sessionExpired = re.findall(
			"Your session has expired, please login again.", err )
		notLoggedIn = re.findall(
			"Perforce password \(P4PASSWD\) invalid or unset.", err )

		if sessionExpired or notLoggedIn:
			raise AuthenticationException, errMessage

		# Any other unknown type of error
		raise RuntimeError, errMessage

	return out
# runConnectedCommand