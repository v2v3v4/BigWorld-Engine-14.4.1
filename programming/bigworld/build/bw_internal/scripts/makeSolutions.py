#!/usr/bin/env python

import os
import sys
import optparse
import subprocess

PY_EXE = "C:\\Python27\\python.exe"

def createSolution(python_exe, cmake, number):
	file = "input.txt"
	f = open(file,'w')

	for num in number.split("-"):
		f.write( num +'\n')
	f.write( '\n')
	
	f.close()
	cmd = python_exe + " " + cmake + " < " + file
	print cmd
	return subprocess.call( cmd, shell=True)


def main():
	python_exe = PY_EXE
	usage = "\npython %prog -c cmake_file.py -n 2,3-1,7\n"
			
	opt = optparse.OptionParser(usage)

	opt.add_option( "-c", "--cmake",
					dest = "cmake", default = None,
					help = "specify location of the cmake file" )
					
	opt.add_option( "-d", "--destination",
					dest = "destination", default = None,
					help = "specify location for the project solution" )
					
	opt.add_option( "-n", "--solNumbers",
					dest = "solNumbers", default = None,
					help = "specify options number separated by a comma and a dash for a second option" )
	
	opt.add_option( "-p", "--python",
					dest = "python", default = None,
					help = "specify python executable (default=" + PY_EXE + ")" )

	(options, args) = opt.parse_args()
	
	if None == options.cmake:
		print "\nPlease specify the location of the cmake file\n"
		opt.print_help()
		return 1
		
	if None == options.solNumbers:
		print "\nPlease specify solution numbers you want to create\n"
		opt.print_help()
		return 1
		
	if not os.path.exists(options.cmake):
		print "\nCant find %s\n", options.cmake
		opt.print_help()
		return 1
		
	os.chdir(os.path.dirname(options.cmake))
	
	if None != options.destination:
		#add a destination to cmake_file.py
		options.cmake += " " + options.destination
		
	if None != options.python:
		if not os.path.exists(options.python):
			print "\nCant find %s\n", options.python
			opt.print_help()
			return 1
		else:
			python_exe = options.python

	builds = (options.solNumbers).split(",")
	buildsNumber = []
	for build in builds:
		number = build
		if "-" in build:
			number = build.split("-")[0]
		try:
			int(number)
			buildsNumber.append(build)
		except:
			print "\n" + build + " is not a number\n\n"
			opt.print_help()
			return 1
			
	error = False
	for number in buildsNumber:
		error = createSolution(python_exe, options.cmake, number)
		if error:
			break
		
	return error


if __name__ == "__main__":
	sys.exit( main() )

# makeRPM.py
