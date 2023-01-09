# A simple script to find the total file sizes for different extensions in a
# directory.

from os.path import join, getsize
import os
import sys

def calcFileSizes( path ):
	totals = {}

	for root, dirs, files in os.walk( path ):
		for name in files:
			size = getsize( join( root, name ) )
			ext = name.split( "." )[-1]
			totals[ ext ] = totals.get( ext, 0 ) + getsize( join( root, name ) )
		try:
			dirs.remove('CVS')  # don't visit CVS directories
		except:
			pass

	sorted = [(b,a) for a, b in totals.items()]
	sorted.sort()
	return sorted

def printFileSizes( path ):
	print "For", os.path.abspath( path )
	sorted = calcFileSizes( path )
	for a, b in sorted:
		print "%15s %15d" % (b, a)
	print
	print "Total:", sum( [a for a,b in sorted] )

if __name__ == "__main__":
	try:
		path = sys.argv[1]
	except:
		path = '.'
	printFileSizes( path )
