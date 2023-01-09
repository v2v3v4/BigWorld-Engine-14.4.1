import struct

class Test( object ):
	def __init__( self, intValue, stringValue, dictValue ):
		self.intValue = intValue
		self.stringValue = stringValue
		self.dictValue = dictValue


def writePascalString( string ):
	return struct.pack( "b", len(string) ) + string


def readPascalString( stream ):
	(length,) = struct.unpack( "b", stream[0] )
	string = stream[1:length+1]
	stream = stream[length+1:]
	return (string, stream)


class TestDataType:

	def addToStream( self, obj ):

		if not obj:
			obj = self.defaultValue()

		stream = struct.pack( "i", obj.intValue )
		stream += writePascalString( obj.stringValue )

		stream += struct.pack( "i", len( obj.dictValue.keys() ) )
		for key in obj.dictValue.keys():
			stream += writePascalString( key )
			stream += writePascalString( obj.dictValue[key] )
		return stream


	def createFromStream( self, stream ):
		(intValue,) = struct.unpack( "i", stream[:4] )
		stream = stream[4:]
		stringValue, stream = readPascalString( stream )
		dictValue = {}

		(dictSize,) = struct.unpack( "i", stream[:4] )
		stream = stream[4:]
		while len( stream ):
			key, stream = readPascalString( stream )
			value, stream = readPascalString( stream )
			dictValue[key] = value

		return Test( intValue, stringValue, dictValue )


	def addToSection( self, obj, section ):

		if not obj:
			obj = self.defaultValue()

		section.writeInt( "intValue", obj.intValue )
		section.writeString( "stringValue", obj.stringValue )
		s = section.createSection( "dictValue" )

		for key in obj.dictValue.keys():
			v = s.createSection( "value" )
			print key, obj.dictValue[key]
			v.writeString( "key", key )
			v.writeString( "value", obj.dictValue[key] )


	def createFromSection( self, section ):
		intValue = section.readInt( "intValue" )

		if intValue is None:
			return self.defaultValue()

		stringValue = section.readString( "stringValue" )
		dictValue = {}

		for value in section["dictValue"].values():
			dictValue[value["key"].asString] = value["value"].asString

		return Test( intValue, stringValue, dictValue )


	def fromStreamToSection( self, stream, section ):
		o = self.createFromStream( stream )
		self.addToSection( o, section )


	def fromSectionToStream( self, section ):
		o = self.createFromSection( section )
		return self.addToStream( o )


	def bindSectionToDB( self, binder ):
		binder.bind( "intValue", "INT32" )
		binder.bind( "stringValue", "STRING", 50 )

		binder.beginTable( "dictValue" )
		binder.bind( "key", "STRING", 50 )
		binder.bind( "value", "STRING", 50 )
		binder.endTable()


	def defaultValue( self ):
		return Test( 100, "Blah", { "happy": "sad", "naughty": "nice",
				"coopers": "carlton" } )

instance = TestDataType()

# TestDataType.py
