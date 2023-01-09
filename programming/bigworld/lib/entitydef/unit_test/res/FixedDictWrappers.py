"""
This module provides a class for holding FixedDict data
and a couple of converters for testing custom streaming
and conversion operations.
"""

import cPickle

class WrapIntStr( object ):
	def __init__( self, data ):
		self.intVal = data[ "int8_5" ]
		self.strVal = data[ "str_TestString" ]


class WrapperWithoutStreaming( object ):
	def createObjFromDict( self, data ):
		return WrapIntStr( data )

	def getDictFromObj( self, obj ):
		return { "int8_5": obj.intVal, "str_TestString": obj.strVal }

	def isSameType( self, obj ):
		return isinstance( obj, WrapIntStr )

class WrapperWithStreaming( WrapperWithoutStreaming ):
	def addToStream( self, obj ):
		return "%u||%s" % ( obj.intVal, obj.strVal )

	def createFromStream( self, stream ):
		intVal, strVal = stream.split( "||", 1 )
		return WrapIntStr( { "int8_5": int(intVal), "str_TestString": strVal } )

noStream = WrapperWithoutStreaming()
stream = WrapperWithStreaming()
