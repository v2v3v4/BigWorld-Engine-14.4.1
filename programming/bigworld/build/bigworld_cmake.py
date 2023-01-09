#!/usr/bin/env python
from __future__ import print_function

import argparse
import getpass
import glob
import os
import subprocess
import sys

if sys.version_info.major > 2:
	raw_input = input

BUILD_DIRECTORY = os.path.dirname( os.path.join( os.getcwd(), __file__ ) )
VC_XP_VARS_BAT = os.path.join( BUILD_DIRECTORY, 'vcxpvarsall.bat' )
SRC_DIRECTORY = os.path.normpath( os.path.join( BUILD_DIRECTORY, ".." ) )
ROOT_DIRECTORY = os.path.join( SRC_DIRECTORY, ".." )
os.chdir( BUILD_DIRECTORY )
CMAKE_RUN_BAT = 'rerun_cmake.bat'
CMAKE_EXE = os.path.join( SRC_DIRECTORY, 'third_party', 'cmake-win32-x86', 'bin', 'cmake.exe' )
PLINK_EXE = os.path.join( SRC_DIRECTORY, 'third_party', 'putty', 'plink.exe' )
	
# Set up MSVC x86 environment with XP support, see
# http://blogs.msdn.com/b/vcblog/archive/2012/10/08/windows-xp-targeting-with-c-in-visual-studio-2012.aspx
VC11_X86_XP_ENV = '@call %s vc11 x86\n' % (VC_XP_VARS_BAT)
VC12_X86_XP_ENV = '@call %s vc12 x86\n' % (VC_XP_VARS_BAT)
VC11_X86_64_XP_ENV = '@call %s vc11 x64\n' % (VC_XP_VARS_BAT)
VC12_X86_64_XP_ENV = '@call %s vc12 x64\n' % (VC_XP_VARS_BAT)

CMAKE_GENERATORS = [
	dict(
		label = 'Visual Studio 2008 Win32',
		generator = 'Visual Studio 9 2008', 
		dirsuffix = 'vc9_win32',
		deprecated = True
	),
	dict(
		label = 'Visual Studio 2010 Win32',
		generator = 'Visual Studio 10', 
		dirsuffix = 'vc10_win32',
		deprecated = True,
		enableForTargets=["bwentity"]
	),
	dict(
		label = 'Visual Studio 2010 Win64',
		generator = 'Visual Studio 10 Win64', 
		dirsuffix = 'vc10_win64',
		deprecated = True,
		enableForTargets=["bwentity"]
	),
	dict(
		label = 'Visual Studio 2012 Win32',
		generator = 'Visual Studio 11', 
		dirsuffix = 'vc11_win32',
		deprecated = True,
		toolset = 'v110_xp'
	),
	dict(
		label = 'Visual Studio 2012 Win64',
		generator = 'Visual Studio 11 Win64', 
		dirsuffix = 'vc11_win64',
		deprecated = True,
		toolset = 'v110_xp'
	),
	dict(
		label = 'Visual Studio 2013 Win32',
		generator = 'Visual Studio 12', 
		dirsuffix = 'vc12_win32',
		deprecated = True,
		toolset = 'v120_xp',
		experimental = True
	),
	dict(
		label = 'Visual Studio 2013 Win64',
		generator = 'Visual Studio 12 Win64', 
		dirsuffix = 'vc12_win64',
		deprecated = True,
		toolset = 'v120_xp',
		experimental = True
	),
	dict(
		label = 'Visual Studio 2019 Win32',
		generator = 'Visual Studio 16 2019',
		architecture = 'Win32',
		dirsuffix = 'vc16_win32',
		toolset = 'v141_xp',
	),
	dict(
		label = 'Visual Studio 2019 Win64',
		generator = 'Visual Studio 16 2019',
		architecture = 'x64',
		dirsuffix = 'vc16_win64',
		toolset = 'v141_xp',
		experimental = True
	),
	dict(
		label = 'Visual Studio 2022 Win32',
		generator = 'Visual Studio 17 2022',
		architecture = 'Win32',
		dirsuffix = 'vc17_win32',
		toolset = 'v141_xp',
	),
	dict(
		label = 'Visual Studio 2022 Win64',
		generator = 'Visual Studio 17 2022',
		architecture = 'x64',
		dirsuffix = 'vc17_win64',
		toolset = 'v141_xp',
		experimental = True
	),
	dict(
		label = 'Ninja with MSVC11 Win32',
		generator = 'CodeBlocks - Ninja', 
		dirsuffix = 'ninja_vc11_win32',
		deprecated = True,
		experimental = True,
		batchenv = VC11_X86_XP_ENV,
		configs = [ 'Debug', 'Hybrid' ]
	),
	dict(
		label = 'Ninja with MSVC11 Win64',
		generator = 'CodeBlocks - Ninja', 
		dirsuffix = 'ninja_vc11_win64',
		deprecated = True,
		experimental = True,
		batchenv = VC11_X86_64_XP_ENV,
		configs = [ 'Debug', 'Hybrid' ]
	),
	dict(
		label = 'Ninja with MSVC12 Win32',
		generator = 'CodeBlocks - Ninja', 
		dirsuffix = 'ninja_vc12_win32',
		deprecated = True,
		experimental = True,
		batchenv = VC12_X86_XP_ENV,
		configs = [ 'Debug', 'Hybrid' ]
	),
	dict(
		label = 'Ninja with MSVC12 Win64',
		generator = 'CodeBlocks - Ninja', 
		dirsuffix = 'ninja_vc12_win64',
		deprecated = True,
		experimental = True,
		batchenv = VC12_X86_64_XP_ENV,
		configs = [ 'Debug', 'Hybrid' ]
	),
	dict(
		label = 'Visual Studio 2012 Win32 with Clang-cl',
		generator = 'Visual Studio 11', 
		dirsuffix = 'vc11_clang_win32',
		deprecated = True,
		toolset = 'LLVM-vs2012_xp',
		experimental = True,
	),
	dict(
		label = 'Visual Studio 2012 Win64 with Clang-cl',
		generator = 'Visual Studio 11 Win64', 
		dirsuffix = 'vc11_clang_win64',
		deprecated = True,
		toolset = 'LLVM-vs2012_xp',
		experimental = True,
	)
]

VERIFY_SERVER = [
	dict( label = 'Yes', value = True ),
	dict( label = 'No' , value = False )
]

def writeCMakeRunBat( cmd, outputDir, batchenv = "", configs = [] ):
	out = open( os.path.join( outputDir, CMAKE_RUN_BAT ), 'w' )

	# write preamble, usually sets up environment
	if batchenv:
		out.write( batchenv )

	cmdstr = ' '.join( cmd )
	if configs:
		print(configs)
		# if single config builder then create subdirs for each config
		for config in configs:
			mkdir = '@if not exist %s\\nul mkdir %s\n' % ( config, config )
			out.write( mkdir )

		for config in configs:
			out.write( '@cd %s\n' % ( config ) )
			out.write( '%s -DCMAKE_BUILD_TYPE=%s\n' % ( cmdstr, config ) )
			out.write( '@cd ..\n' )
	else:
		# otherwise write out single command
		out.write( cmdstr )

	# close file before running it!
	out.write('\n')
	out.close()

	# run the batch file
	process = subprocess.Popen( ['cmd.exe', '/C', CMAKE_RUN_BAT ],
		cwd=outputDir )
	process.communicate()


def targetChoices():
	def _stripName( n ):
		return n.replace( r"cmake\BWConfiguration_", "" ).replace( ".cmake", "" )
	targets = [ _stripName(x) for x in glob.glob( "cmake/BWConfiguration_*.cmake" ) ]
	targets.sort()
	return targets


def findCMakeTargets():
	items = []
	for target in targetChoices():
		items.append( { "label" : target } )
	return items


def generatorChoices():
	generators = []
	for generator in CMAKE_GENERATORS:
		generators.append( generator['dirsuffix'] )
	generators.sort()
	return generators


def chooseItem( prompt, items, deprecated = False, experimental = False, targets = None ):

	# filter out items we aren't interested in
	def displayItem( item ):
		if targets:
			for target in targets:
				if target in item.get( 'enableForTargets', [] ):
					return True
		if item.get( 'deprecated', False ) and not deprecated:
			return False
		if item.get( 'experimental', False ) and not experimental:
			return False
		return True
	items = list(filter(displayItem, items))

	if len(items) == 1:
		return items[0]

	print(prompt)
	for i in range( len(items) ):
		item = items[i]
		label = item['label']
		print('\t%d. %s' % (i + 1, label))
	choice = None
	while choice is None:
		try:
			choice = int(raw_input('> '))
			choice -= 1
			if choice < 0 or choice >= len( items ):
				choice = None
		except ValueError:
			choice = None
		
		if choice is None:
			print("Invalid option")
	return items[choice]


def editableList( prompt, items ):
	while True:
		print(prompt)
		for i in range( len(items) ):
			item = items[i]
			label = item[0]
			value = item[1]
			if type( value ) is tuple:
				value = value[0]
			print("\t%d. %s = %s" % (i+1, label, value))
		choice = None

		try:
			choice = int(raw_input('> '))
			choice -= 1
			if choice < 0 or choice >= len(items):
				choice = None
		except ValueError:
			choice = None

		if choice is None:
			break

		if len(items[choice]) == 2:
			items[choice][1] = raw_input(items[choice][0] + "> ")
		else:
			items[choice][1] = chooseItem(items[choice][0], items[choice][2])


def testPlinkConnection( username, hostname, privateKeyPath, linuxPath ):
	hasValidConfig = False
	try:
		plinkArgs = [PLINK_EXE, '-batch', '-v']
		if privateKeyPath == "":
			plinkArgs.append( "-agent" )
		else:
			plinkArgs += ['-i', '%s' % (privateKeyPath)]
		plinkArgs.append( r'%s@%s' % (username, hostname) )
		plinkArgs += ['if [[ -f %s/CMakeLists.txt && -f %s/Makefile && -d %s/build ]]; then echo CMAKE_SUCCESS; else echo CMAKE_BAD_PATH; fi' % ( linuxPath, linuxPath, linuxPath )]
		print(plinkArgs)
		plink = subprocess.Popen( plinkArgs, cwd=BUILD_DIRECTORY,
				universal_newlines=True, stdin=subprocess.PIPE,
				stdout=subprocess.PIPE, stderr=subprocess.PIPE )
		# We use stdin so plink doesn't steal our stdin
		plink.stdin.close()
		plink.wait()
		stdout = plink.stdout.read()
		stderr = plink.stderr.read()
		plink.stdout.close()
		plink.stderr.close()
		if stdout.strip() == "CMAKE_SUCCESS":
			hasValidConfig = True
			print("Managed to establish connection")
		elif stdout.strip() == "CMAKE_BAD_PATH":
			print("Incorrect mount path")
		else:
			print(stderr)
	except OSError as e:
		print("Unable to execute plink %s" % str(e))
	return hasValidConfig



def getServerOptions(allowRsync = False):
	hostname = raw_input('Hostname> ')
	server_directory = raw_input('Linux "programming/bigworld" path> ')

	CONNECTION_TYPES = [
		dict( label = "Putty SSH Session to Windows Mount", value = "PUTTY_SSH" ),
		]

	connection_type = chooseItem( "How do you wish to connect to the server?", 
			CONNECTION_TYPES )

	private_key_path = raw_input('Private key path if needed> ')

	hasValidConfig = False

	serverOptions = [
		["Connection type", connection_type["value"], CONNECTION_TYPES],
		["Username", getpass.getuser()],
		["Hostname", hostname],
		["Private key path", private_key_path],
		["Linux source path", server_directory],
		["Compile Flags", ""],
		["Replace Paths in Output", "ON", ["ON","OFF"]]
	]
	
	serverOptionsKeys = {
		'connectionType': 0,
		'username': 1,
		'hostname': 2,
		'privateKeyPath': 3,
		'linuxPath': 4,
		'compileFlags': 5,
		'replacePathsInOutput': 6,
		}
		
	def opt(key):
		return serverOptions[serverOptionsKeys[key]][1]
	
	while not hasValidConfig:
		editableList("Server Options (Leave Blank to continue)", serverOptions)
		
		if opt('connectionType') == "PUTTY_SSH":
			print("Testing PuTTY connection")
			hasValidConfig = testPlinkConnection( opt('username'),
				opt('hostname'), opt('privateKeyPath'), opt('linuxPath') )

	return [
				'-DBW_LINUX_CONN_TYPE="%s"' % (opt('connectionType')),
				'-DBW_LINUX_USER="%s"' % (opt('username')),
				'-DBW_LINUX_HOST="%s"' % (opt('hostname')),
				'-DBW_LINUX_PRIV_KEY="%s"' % (opt('privateKeyPath')),
				'-DBW_LINUX_DIR="%s"' % (opt('linuxPath')),
				'-DBW_LINUX_CFLAGS="%s"' % (opt('compileFlags')),
				'-DBW_REPLACE_LINUX_DIR="%s"' % (opt('replacePathsInOutput'))
			]


def serverOpts( targetName, generator, args ):
	# check for server builds
	opts = [ '-DBW_VERIFY_LINUX=OFF' ]
	if targetName == 'server':
		generator['dirsuffix'] = 'linux'
		opts = getServerOptions(False)
	elif args.show_verify_server:
		verifyServer = chooseItem(
				'Would you like to add server compile pre-build steps?',
				VERIFY_SERVER )
		if verifyServer['value']:
			opts = getServerOptions(True)
			opts.append( '-DBW_VERIFY_LINUX=ON' )
	return opts


def generate( targetName, generator, cmakeExe, cmakeOpts, buildDir, dryRun ):

	# create output directory
	outputDir = os.path.normpath( os.path.join( buildDir,
		('build_%s_%s' % (targetName, generator['dirsuffix'])).lower() ) )
	if not os.path.isdir( outputDir ):
		os.makedirs( outputDir )

	# generate cmake command list
	cmd = [
		cmakeExe, SRC_DIRECTORY,
		'-Wno-dev', # disable CMakeLists.txt developer warnings
		'-G"%s"' % generator['generator'],
		'-DBW_CMAKE_TARGET=%s' % targetName
	]

	# optionally append toolset
	if ('toolset' in generator):
		cmd.append( '-T' )
		cmd.append( generator['toolset'] )

	# optionally append arch
	if ('architecture' in generator):
		cmd.append( '-A' )
		cmd.append( generator['architecture'] )

	# append server build options
	if cmakeOpts:
		cmd = cmd + cmakeOpts

	# configs to build for single config builders
	if ('configs' in generator):
		configs = generator['configs']
		# hardcode consumer release for client configs...
		if targetName == 'client' or targetName == 'client_blob':
			configs.append( 'Consumer_Release' )

	# output out command
	print
	print(']', ' '.join( cmd ) )

	# write and execute the cmake run bat file
	if not dryRun:
		writeCMakeRunBat( cmd, outputDir, generator.get( 'batchenv', '' ),
			generator.get( 'configs', [] ) )


def main():
	# parse command line options
	parser = argparse.ArgumentParser()
	parser.add_argument( 'builddir', nargs='?', type=str,
			default=ROOT_DIRECTORY, help='set root build directory' )
	parser.add_argument( '--target', action='append',
			choices=targetChoices(),
			help='generate the specified target' )
	parser.add_argument( '--generator', action='append',
			choices=generatorChoices(),
			help='use the specified generator' )
	parser.add_argument( '--experimental', action='store_true',
			help='show experimental options' )
	parser.add_argument( '--deprecated', action='store_true',
			help='show deprecated options' )
	parser.add_argument( '--cmake-exe', type=str,
			help='specify a CMake exe to use' )
	parser.add_argument( '--show-verify-server', action='store_true',
			help='show server build verification option' )
	parser.add_argument( '--dry-run', action='store_true',
			help='query build options but don\'t run CMake')
	args = parser.parse_args()

	targets = []
	generators = []

	# choose target project
	if args.target is None:
		targetItems = findCMakeTargets()
		target = chooseItem( "What do you want to build?", targetItems )
		targets = [ target['label'] ]
	else:
		targets = args.target

	# choose generator
	if args.generator is None:
		generators = [ chooseItem( "What do you want to build with?",
				CMAKE_GENERATORS, args.deprecated, args.experimental,
				targets = targets ) ]
	else:
		for generator in args.generator:
			for cmake_generator in CMAKE_GENERATORS:
				if cmake_generator['dirsuffix'] == generator:
					generators.append( cmake_generator )
		assert( len(generators) != 0 )
	
	# set cmake executable
	if args.cmake_exe:
		cmakeExe = os.path.normpath( args.cmake_exe )
	else:
		cmakeExe = CMAKE_EXE
	cmakeExe = '"%s"' % ( cmakeExe, )

	for generator in generators:
		for target in targets:
			cmakeOpts = serverOpts( target, generator, args )
			generate( target, generator, cmakeExe, cmakeOpts, args.builddir,
					args.dry_run );


if __name__ == '__main__':
	main()

