import BigWorld
import random

# ------------------------------------------------------------------------------
# Section: class Guard
# ------------------------------------------------------------------------------

class Guard( BigWorld.Base ):

	def __init__( self ):
		BigWorld.Base.__init__( self )
		print "Base: Guard.__init__"
		self.cellData[ "position" ] = (random.randrange( -1, 1 ), 0,
				random.randrange( -1, 1 ))
								
		self.createCellEntity( BigWorld.globalBases[ "DefaultSpace" ].cell )

# Guard.py
