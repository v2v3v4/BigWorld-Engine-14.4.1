"""
Base personality script.
"""

import BigWorld

# This is to avoid loading in the main thread later
import TestDataType

def onBaseAppReady( isBootstrap, didAutoLoadEntitiesFromDB ):
	if isBootstrap:
		BigWorld.createBaseLocally( "Space" )

# egclient4.py
