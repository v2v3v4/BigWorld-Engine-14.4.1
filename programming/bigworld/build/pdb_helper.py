#!/usr/bin/env python
import argparse
import copy
import getpass
import os
import re
import subprocess

# Default branch root from this script
ROOT_DIR = os.path.normpath(os.path.join(os.getcwd(), '../../..'))

# Default source root directory
SOURCE_ROOT_DIR = os.path.join(ROOT_DIR, 'programming', 'bigworld')

# Symbol store network share
SYMDST_DIR = '\\\\au1-mss\\bwsymbols'

# Source indexing ini file path
SRCSRV_INI= os.path.join(SYMDST_DIR, 'srcsrv.ini')

# Strawberry Perl exe location
PERL_DIR = 'C:\\strawberry\\perl\\bin'
PERL_EXE = os.path.join(PERL_DIR, 'perl.exe')

# Locations to search for WDK debugging tools
def wdk_search_dirs():
	wdkdirs = []
	# order here is search for x64 version first
	search_dirs = [os.environ['ProgramW6432'],
			os.environ['ProgramFiles(x86)']]
	for arch in ['x64', 'x86']:
		for search_dir in search_dirs:
			wdkdir = os.path.normpath(os.path.join(search_dir,
				'Windows Kits', '8.1', 'Debuggers', arch))
			if os.path.isdir(wdkdir):
				wdkdirs.append(wdkdir)
	return wdkdirs

# Get path to symstore.exe
def find_symstore_exe():
	search_dirs = wdk_search_dirs()
	for search_dir in search_dirs:
		filepath = os.path.normpath(os.path.join(search_dir, 'symstore.exe'))
		if os.path.isfile(filepath):
			return filepath
	return None

# Get path to p4index.cmd
def find_p4index_cmd():
	search_dirs = wdk_search_dirs()
	for search_dir in search_dirs:
		filepath = os.path.normpath(os.path.join(search_dir,
			'srcsrv', 'p4index.cmd'))
		if os.path.isfile(filepath):
			return filepath
	return None

# Get the path to perl
def find_perl_exe():
	if os.path.isfile(PERL_EXE):
		return PERL_EXE
	return None

# Index asset pipeline symbol files
def index_symbol_sources(src_dir, bin_dir, ini_file, p4_ticket, quiet, dry_run):
	env = copy.deepcopy(os.environ)
	p4index_cmd = find_p4index_cmd()
	# Check perl is in the path, if not then add it
	if subprocess.call(['where', 'perl.exe']) != 0:
		perl_exe = find_perl_exe()
		if perl_exe:
			perl_path = os.path.dirname(perl_exe)
			path = env['PATH'] + os.pathsep + perl_path
			env['PATH'] = path
			if not quiet:
				print('PATH=%s' % env['PATH'])
	# Set P4PASSWD if we've been given a p4 login ticket
	if p4_ticket:
		env['P4PASSWD'] = p4_ticket
		if not quiet:
			print('P4PASSWD=%s' % env['P4PASSWD'])
	cmd = [p4index_cmd,
			'-Ini=%s' % os.path.normpath(ini_file),
			'-Source=%s' % os.path.normpath(src_dir), 
			'-Symbols=%s' % os.path.normpath(bin_dir)]
	if not quiet:
		cmd.append('/debug')
	if not quiet or dry_run:
		print(' '.join(cmd))
	if not dry_run:
		p = subprocess.Popen(cmd, env=env)
		p.communicate()

# Store asset pipeline symbols on the symbol server
# See http://msdn.microsoft.com/en-us/library/windows/desktop/ms681378%28v=vs.85%29.aspx
# for detailed help on the SymStore command line options
def store_symbols(bin_dir, sym_dir, nocopy, product_name, product_version,
		comment, quiet, dry_run):
	symstore_exe = find_symstore_exe()
	cmd = [symstore_exe, 'add',
			# recurse directories
			'/r',
			# files or directories to add
			'/f', os.path.join(os.path.normpath(bin_dir), '*.*'),
			# root directory for symbol store
			'/s', os.path.normpath(sym_dir),
			# product name
			'/t', product_name,
			# product version
			'/v', product_version
			]
	if comment:
		# specify comment for the transaction
		cmd.append('/c')
		cmd.append(comment)
	if nocopy:
		# store pointer to file rather than actual file
		cmd.append('/p')
	if not quiet:
		# display verbose output
		cmd.append('/o')
	if dry_run or not quiet:
		print(' '.join(cmd))
	if not dry_run:
		subprocess.check_call(cmd)

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--quiet', action='store_true',
			help='Suppress version output')
	parser.add_argument('--dry-run', action='store_true',
			help='don\'t execute actual commands')
	subparser = parser.add_subparsers(dest='command')

	index_parser = subparser.add_parser('index',
			help='Index PDB sources')
	index_parser.add_argument('--p4-ticket',
			help='Perforce login ticket')
	index_parser.add_argument('--source-dir',
			default=SOURCE_ROOT_DIR,
			help='Root directory of source files')
	index_parser.add_argument('--ini-path',
			default=SRCSRV_INI,
			help='Path to the SRCSRV ini file')
	index_parser.add_argument('bin_dir',
			help='Path to PDB files')

	store_parser = subparser.add_parser('store',
			help='Store PDBs on Symbol Server')
	store_parser.add_argument('--product-name', required=True,
			help='The name of the product')
	store_parser.add_argument('--product-version', required=True,
			help='The version of the product')
	store_parser.add_argument('--comment',
			default=None,
			help='A comment for the transaction')
	store_parser.add_argument('--server-dir',
			default=SYMDST_DIR,
			help='The root directory for the symbol store')
	store_parser.add_argument('--nocopy', action='store_true',
			help='store a pointer to the file rather than copying the file')
	store_parser.add_argument('bin_dir',
			help='Path to PDB files')
	
	args = parser.parse_args()

	if args.command == 'index':
		index_symbol_sources(args.source_dir, args.bin_dir, args.ini_path,
				args.p4_ticket, args.quiet, args.dry_run)
	elif args.command == 'store':
		store_symbols(args.bin_dir, args.server_dir, args.nocopy,
				args.product_name, args.product_version, args.comment,
				args.quiet, args.dry_run)

if __name__ == '__main__':
	main()

