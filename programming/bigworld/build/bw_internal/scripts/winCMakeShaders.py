#!/usr/bin/env python

import os
import sys
import build_common

SHADER_DIR = os.path.join(
	build_common.getRootPath(), "..", "game", "bin", "tools", "misc", "rebuild_shaders" )


def buildShaders():
	origDir = os.getcwd()

	os.chdir( SHADER_DIR )

	# NB: Force this execution to use the system version of Python.
	# It appears to fail in some circumstances otherwise. The root cause has
	# not been identified yet.
	build_common.runCmd( sys.executable + " RebuildShaders.py -a" )

	os.chdir( origDir )

	return True

def main():
	return buildShaders()

if __name__ == "__main__":
	sys.exit( not main() )

# winMakeShaders.py
