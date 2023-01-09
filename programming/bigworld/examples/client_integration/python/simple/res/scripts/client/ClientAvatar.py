"""
The ClientAvatar Client Entity script. 

Implements the client method fromCell() which is called from the ClientAvatar
cell entity.
"""
import BigWorld

# ------------------------------------------------------------------------------
# Section: class ClientAvatar
# ------------------------------------------------------------------------------
class ClientAvatar( BigWorld.Entity ):

	def __init__( self ):
		BigWorld.Entity.__init__( self )
		print "ClientAvatar.__init__:"
		print self.__dict__


	def onFinish( self ):
		self.base.logOff()


	def fromCell( self, msg ):
		print "ClientAvatar.fromCell:", self.id, ":", msg
		self.createLocalEntity()


	def createLocalEntity( self ):
		localEntityID = BigWorld.createEntity( 
				"ClientOnly",
				self.spaceID, 0, 
				self.position, (self.roll, self.pitch, self.yaw),
				{} )
		print "ClientAvatar.createLocalEntity: localEntityID:", localEntityID
		localEntity = BigWorld.entities[ localEntityID ]
		localEntity.method1( "a random message" )
		self._localEntityID = localEntityID


	def tickAndDestroyLocalEntity( self ):
		if hasattr( self, "_tickCount" ):
			self._tickCount += 1
			if self._tickCount == 5:
				res = BigWorld.destroyEntity( self._localEntityID )
				print "ClientAvatar.tickAndDestroyLocalEntity: res = ", res
		elif  hasattr( self, "_localEntityID" ):
			self._tickCount = 0 
		 

	def cellTick( self, t ):
		self.tickAndDestroyLocalEntity()
#		print "tick: ", t
#		print self.prop5.intValue
		return

# ClientAvatar.py
