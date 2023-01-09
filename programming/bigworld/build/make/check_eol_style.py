#!/usr/bin/env python

import os
import pysvn
import re
import sys

import optparse

ENFORCE_NATIVE = False

PATTERN = r"\.[chi](pp)?$"
EOL_PROP_NAME = "svn:eol-style"

SRC_DIR = os.path.abspath(
			os.path.join(
				os.path.dirname( os.path.abspath( __file__ ) ),
				"../" ) )
MF_ROOT = os.path.abspath( os.path.join( SRC_DIR, "../../" ) ) + "/"

def getPath( fullPath ):
	return fullPath.split( MF_ROOT )[ -1 ]

class EOLChecker( object ):
	def __init__( self, regularExpression, shouldFix, quiet ):
		self.hasFailures = False
		self.svn = pysvn.Client()
		print "regularExpression is", regularExpression
		self.regularExpression = re.compile( regularExpression )
		self.shouldFix = shouldFix
		self.quiet = quiet

	def __call__( self, arg, dirname, fnames ):
		try:
			fnames.remove( ".svn" )
		except ValueError:
			pass

		for fname in fnames:
			# Skip symbolic links as the link itself can have properties
			fullFilePath = os.path.join( dirname, fname )
			if os.path.islink( fullFilePath ):
				continue

			# If it looks like a source file, check the eol-style
			if self.regularExpression.search( fname ):
				try:
					props = self.svn.propget( EOL_PROP_NAME, fullFilePath )
				except pysvn.ClientError:
					# some source files are not versioned (eg: buildinf.h)
					continue

				if len( props ) != 1:
					if self.shouldFix:
						try:
							self.svn.propset( EOL_PROP_NAME, "native", fullFilePath )
						except pysvn.ClientError, e:
							print "Failed for %s (%s)", fullFilePath, str( e )

					if not self.quiet:
						print "No eol-style:", getPath( fullFilePath )
					self.hasFailures = True

				elif ENFORCE_NATIVE and props.get( fullFilePath ) != "native":
					print "Incorrect eol-style '%s' on: %s" % \
						( props.get( EOL_PROP_NAME ), getPath( fullFilePath ) )
					self.hasFailures = True


def main():
	parser = optparse.OptionParser()
	parser.add_option( "-m", "--match", dest="pattern", default=PATTERN )
	parser.add_option( "-f", "--fix", action = "store_true", dest="fix" )
	parser.add_option( "-q", "--quiet", action = "store_true", dest="quiet" )
	parser.add_option( "-d", "--dir", dest="directory", default=SRC_DIR )
	(options, args) = parser.parse_args( sys.argv[1:] )

	visitor = EOLChecker( regularExpression = options.pattern,
			shouldFix = options.fix,
		   	quiet = options.quiet )
	os.path.walk( options.directory, visitor, None )

	return int( visitor.hasFailures )

if __name__ == "__main__":
	sys.exit( main() )

# check_eol_style.py
