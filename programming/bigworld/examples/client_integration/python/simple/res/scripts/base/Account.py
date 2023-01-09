"""
The Account Base Entity script.

This entity is created when the client logs on as MyAccount. It has no
cell part.

When the client enables entity updates after logging on, the callback
onEntitiesEnabled is called, which creates a periodic timer, such that every
second the callback onTimer is called. After five consecutive calls,
a ClientAvatar entity with the same name as the MyAccount's
'character' property will be created from the database.

In a real game, some sort of selection mechanism through a UI can be presented
for the user to select a particular character for this account, but for now we
achieve character selection by setting the 'character' property.

When the corresponding ClientAvatar (also a BigWorld.Proxy subclass) is
created, it passes off the client connection to the new ClientAvatar entity
by using giveClientTo() and destroys itself.

When giveClientTo() is called, the client is given a message to reset its
entities, and eventually the C++ callback
ServerMessageHandler::onBasePlayerCreate will be called again.
"""

import BigWorld

# ------------------------------------------------------------------------------
# Section: class Account
# ------------------------------------------------------------------------------

class Account( BigWorld.Proxy ):

	def __init__( self ):
		BigWorld.Proxy.__init__( self )
		print "Account.__init__:"
		self.count = 0


	def onClientDeath( self ):
		print "Account.onClientDeath:", self.id
		self.destroy()


	def onEntitiesEnabled( self ):
		"""
		This callback is called when the client signals that it is ready to
		receive entity updates after logging in
		"""
		print "Account.onEntitiesEnabled:", self.id
		self.addTimer( 1.0, 1.0 )


	def onTimer( self, timerId, userData ):
		self.count += 1
		print "Account.onTimer:", self.count
		self.client.chatMessage( "Hello from the base: " + str( self.count ) )

		if self.count == 5:
			# 5 seconds has passed - remove the timer now that it is not needed
			self.delTimer( timerId )
			# Create an instance of the corresponding ClientAvatar entity
			# with the name from the 'character' property and
			# call back on handleNewCharacter()
			print "Loading", self.character
			BigWorld.createBaseFromDB( "ClientAvatar", self.character,
				self.handleNewCharacter )


	def handleNewCharacter( self, newEntity, dbID, wasActive ):
		"""
		Callback when the database has loaded the corresponding ClientAvatar
		entity.

		newEntity is either None if the loading failed, or the ClientAvatar
		entity.
		"""
		print "Account.handleNewCharacter:", self.id
		if newEntity != None:
			# we have loaded the ClientAvatar entity!
			# The client should now talk to that entity, and we can
			# destroy ourself
			self.giveClientTo( newEntity )
			self.destroy()
		else:
			print "Failed to create new entity"
			self.client.chatMessage( "Failed to load the character" )



# Account.py
