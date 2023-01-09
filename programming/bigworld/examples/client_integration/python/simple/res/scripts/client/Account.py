"""
The Account Client Entity script.

This entity is created when the client logs in as MyAccount, but is destroyed
when the base calls givesClientTo the corresponding ClientAvatar character 
entity.
"""
import BigWorld

# ------------------------------------------------------------------------------
# Section: class Account
# ------------------------------------------------------------------------------

class Account( BigWorld.Entity ):
	def __init__( self ):
		BigWorld.Entity.__init__( self )
		print "Client: Account.__init__"
		print self.__dict__
		print dir(self)
		self.tick = 0

	def chatMessage( self, msg ):
		print "Account.chatMessage:", msg

	def onTick( self, t ):
#		print "Account.onTick"
		self.tick += 1

	def onFinish( self ):
		pass

# Account.py
