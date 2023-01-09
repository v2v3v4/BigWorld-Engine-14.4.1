import BigWorld

class Guard( BigWorld.Entity ):
	def __init__( self ):
		BigWorld.Entity.__init__( self )
		print "Guard.__init__:"
		print self.__dict__

	def onTick( self, t ):
		pass

# Guard.py
