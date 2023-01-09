"""
The ClientOnly Client Entity script. 

Implements an example of client-only entity
"""
import BigWorld

# ------------------------------------------------------------------------------
# Section: class ClientOnly
# ------------------------------------------------------------------------------
class ClientOnly( BigWorld.Entity ):
	def __init__( self ):
		BigWorld.Entity.__init__( self )
		print "ClientOnly.__init__:"
		print self.__dict__

	def onFinish( self ):
		pass

	def method1( self, msg ):
		print "ClientOnly.method1:", self.id, ":", msg

	def onTick( self, t ):
		return
#		print t
#		print self.prop5.intValue

# ClientOnly.py
