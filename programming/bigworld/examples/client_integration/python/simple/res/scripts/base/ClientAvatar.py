"""
The ClientAvatar Base Entity script.

This entity is the character avatar entity. When it is created on the base,
it will wait for the client to signal enabling updates from the cell via the
enableEntities call - this occurs when the client logs in, for example - and 
then create the ClientAvatar cell in the default space. 
"""

import BigWorld

# ------------------------------------------------------------------------------
# Section: class ClientAvatar
# ------------------------------------------------------------------------------

class ClientAvatar( BigWorld.Proxy ):

	def __init__( self ):
		BigWorld.Proxy.__init__( self )
		print "ClientAvatar.__init__:", self.__dict__, self.databaseID


	def onEntitiesEnabled( self ):
		print "ClientAvatar.onEntitiesEnabled:", self.id
		self.createCellEntity( BigWorld.globalBases[ "DefaultSpace" ].cell )


	def onClientDeath( self ):
		print "ClientAvatar.onClientDeath:", self.id
		if hasattr( self, "cell" ):
			self.destroyCellEntity()
		else:
			self.destroy()


	def logOff( self ):
		print "ClientAvatar.logOff:", self.id
		if hasattr( self, "cell" ):
			self.destroyCellEntity()
		else:
			self.destroy()


	def onLoseCell( self ):
		self.destroy()

# ClientAvatar.py
