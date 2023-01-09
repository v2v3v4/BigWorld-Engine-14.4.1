import BigWorld

class Space( BigWorld.Base ):

	def __init__( self ):
		BigWorld.Base.__init__( self )

		# Create this entity in a new space
		self.createInNewSpace()

		self.registerGlobally( "DefaultSpace", self.onRegistered )

	def onRegistered( self, succeeded ):
		if not succeeded:
			print "Failed to register space."
			self.destroyCellEntity()

	def onLoseCell( self ):
		self.destroy()

# Space.py
