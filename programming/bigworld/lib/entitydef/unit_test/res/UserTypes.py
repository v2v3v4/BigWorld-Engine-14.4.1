"""
This module provides a class for holding some data
and a couple of converters for testing custom streaming
and conversion operations.
"""

class IntStr( object ):
	def __init__( self, intVal, strVal ):
		self.intVal = int( intVal )
		self.strVal = strVal


class IntStrType( object ):
	def addToStream( self, obj ):
		return "%u||%s" % ( obj.intVal, obj.strVal )

	def createFromStream( self, stream ):
		intVal, strVal = stream.split( "||", 1 )
		return IntStr( intVal, strVal )

	def addToSection( self, obj, section ):
		section.writeInt( "intVal", obj.intVal )
		section.writeString( "strVal", obj.strVal )

	def createFromSection( self, section ):
		return IntStr( section._intVal.asInt, section._strVal.asString )

	def fromStreamToSection( self, stream, section ):
		obj = self.createFromStream( stream )
		self.addToSection( obj, section ) 

	def fromSectionToStream( self, section ):
		obj = self.createFromSection( section )
		return self.addToStream( obj ) 

	def defaultValue( self ):
		return IntStr( 0, "" )

intStr = IntStrType()
