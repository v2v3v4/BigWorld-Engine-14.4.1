#ifdef SCRIPT_OBJECTIVE_C

#import <Foundation/Foundation.h>

#import "oc_script_object.hpp"

#import "math/vector2.hpp"
#import "math/vector3.hpp"
#import "math/vector4.hpp"

// -----------------------------------------------------------------------------
// Section: ScriptObject
// -----------------------------------------------------------------------------

ScriptObject::ScriptObject( void * pObject, bool alreadyIncremented ) :
	pNSObj_( pObject )
{
	if (!alreadyIncremented)
	{
		[(id)pNSObj_ retain];
	}
}

ScriptObject::ScriptObject( const ScriptObject & other ) :
	pNSObj_( other.get() )
{
	[(id)pNSObj_ retain];
}

ScriptObject::~ScriptObject()
{
	[(id)pNSObj_ release];
}

ScriptObject & ScriptObject::operator=( const ScriptObject & other )
{
	if (pNSObj_ != other.pNSObj_)
	{
		[(id)pNSObj_ release];
		pNSObj_ = other.pNSObj_;
		[(id)pNSObj_ retain];
	}

	return *this;
}


ScriptObject ScriptObject::none()
{
	return nil;
}


bool ScriptObject::isNone() const
{
	return (id)pNSObj_ == nil;
}

int ScriptObject::compareTo( const ScriptObject & other ) const
{
	id x = (id)pNSObj_;
	id y = (id)other.pNSObj_;

    return int( [x compare:y] );
}

const char * ScriptObject::typeNameOfObject() const
{
    return [NSStringFromClass( [id(pNSObj_) class] ) UTF8String];
}

// -----------------------------------------------------------------------------
// Section: ScriptDict
// -----------------------------------------------------------------------------

bool ScriptDict::check( const ScriptObject & object )
{
	id obj = (id)object.get();
	
	return [obj isKindOfClass:[NSDictionary class]];
}


ScriptDict ScriptDict::create( int capacity )
{
	return [NSMutableDictionary dictionaryWithCapacity:capacity];
}

bool ScriptDict::setItemString( const char * key, ScriptObject sObj )
{
	NSMutableDictionary * dict = (NSMutableDictionary *)pNSObj_;
	[dict setObject: (id)sObj.get() forKey: [NSString stringWithUTF8String:key]];
	
	return true;
}

ScriptObject ScriptDict::getItemString( const char * key ) const
{
	NSDictionary * dict = (NSDictionary *)pNSObj_;
	return [dict objectForKey: [NSString stringWithUTF8String: key] ];

}

int ScriptDict::size() const
{
	NSDictionary * dict = (NSDictionary *)pNSObj_;
	return [dict count];
}

// -----------------------------------------------------------------------------
// Section: ScriptTuple
// -----------------------------------------------------------------------------

int ScriptSequence::size() const
{
	return [(NSArray *)this->get() count];
}

bool ScriptSequence::check( const ScriptObject & object )
{
	id obj = (id)object.get();
	
	return [obj isKindOfClass:[NSArray class]];
}

ScriptObject ScriptSequence::getItem( int pos ) const
{
	return [(id)pNSObj_ objectAtIndex:pos];
}

bool ScriptSequence::setItem( int pos, ScriptObject item )
{
	[(id)pNSObj_ replaceObjectAtIndex:pos withObject:(id)item.get()];

	return true;
}


// -----------------------------------------------------------------------------
// Section: ScriptList
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Section: ScriptTuple
// -----------------------------------------------------------------------------

	
ScriptTuple ScriptTuple::create( int len )
{
	NSMutableArray * newArray = [NSMutableArray arrayWithCapacity:len];

	for (int i = 0; i < len; ++i)
	{
		[newArray addObject:nil];
	}
	
	return newArray;
}

	
bool ScriptList::check( ScriptObject object )
{
	id obj = (id)object.get();
	
	return [obj isKindOfClass:[NSArray class]];
}

ScriptList ScriptList::create( int capacity )
{
	return [NSMutableArray arrayWithCapacity: capacity];
}

	
bool ScriptList::append( ScriptObject object )
{
	[(id)pNSObj_ addObject:(id)object.get()];

	return true;
}


// -----------------------------------------------------------------------------
// Section: ScriptString
// -----------------------------------------------------------------------------

bool ScriptString::check( ScriptObject object )
{
	id obj = (id)object.get();
	
	return [obj isKindOfClass:[NSString class]] ||
			[obj isKindOfClass:[NSData class]];	
}

// -----------------------------------------------------------------------------
// Section: ScriptBlob
// -----------------------------------------------------------------------------

bool ScriptBlob::check( const ScriptObject & object )
{
	return [(id)object.get() isKindOfClass:[NSData class]];
}


ScriptBlob ScriptBlob::create( const std::string & blobStr )
{
	return [NSData dataWithBytes: blobStr.data() length:blobStr.size()];
}

std::string ScriptBlob::asString() const
{
	NSData * obj = (id)this->get();
	return std::string( (const char *)[obj bytes], [obj length] );
}

extern bool g_shouldDebug;

// -----------------------------------------------------------------------------
// Section: Conversion
// -----------------------------------------------------------------------------

ScriptObject createScriptObjectFrom( double value )
{
	return [NSNumber numberWithDouble:value];
}

ScriptObject createScriptObjectFrom( float value )
{
	return [NSNumber numberWithFloat:value];
}

ScriptObject createScriptObjectFrom( int value )
{
	return [NSNumber numberWithInt:value];
}

ScriptObject createScriptObjectFrom( unsigned int value )
{
	return [NSNumber numberWithUnsignedInt:value];
}

ScriptObject createScriptObjectFrom( long value )
{
	return [NSNumber numberWithLong:value];
}

ScriptObject createScriptObjectFrom( unsigned long value )
{
	return [NSNumber numberWithUnsignedLong:value];
}

ScriptObject createScriptObjectFrom( long long value )
{
	return [NSNumber numberWithLongLong:value];
}

ScriptObject createScriptObjectFrom( unsigned long long value )
{
	return [NSNumber numberWithUnsignedLongLong:value];
}

ScriptObject createScriptObjectFrom( const std::string & value )
{
	return [NSString stringWithUTF8String: value.c_str()];
	// NOTE: This is a blob
	// return [NSData dataWithBytes:value.data() length:value.size()];
}

ScriptObject createScriptObjectFrom( const char * value )
{
	// NOTE: This must be a null terminated UTF8 string
	return [NSString stringWithUTF8String:value];
}

ScriptObject createScriptObjectFrom( const Vector2 & value )
{
	return [NSArray arrayWithObjects:
			(id)createScriptObjectFrom( value[0] ).get(),
			(id)createScriptObjectFrom( value[1] ).get(),
			nil];
}

ScriptObject createScriptObjectFrom( const Vector3 & value )
{
	return [NSArray arrayWithObjects:
			(id)createScriptObjectFrom( value[0] ).get(),
			(id)createScriptObjectFrom( value[1] ).get(),
			(id)createScriptObjectFrom( value[2] ).get(),
			nil];
}

ScriptObject createScriptObjectFrom( const Vector4 & value )
{
	return [NSArray arrayWithObjects:
			(id)createScriptObjectFrom( value[0] ).get(),
			(id)createScriptObjectFrom( value[1] ).get(),
			(id)createScriptObjectFrom( value[2] ).get(),
			(id)createScriptObjectFrom( value[3] ).get(),
			nil];
}


// -----------------------------------------------------------------------------
// Section: convertScriptObjectTo
// -----------------------------------------------------------------------------

bool convertScriptObjectTo( ScriptObject object, double & rValue )
{
	rValue = [(id)object.get() doubleValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, float & rValue )
{
	rValue = [(id)object.get() floatValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, int & rValue )
{
	rValue = [(id)object.get() intValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, unsigned int & rValue )
{
	rValue = [(id)object.get() unsignedIntValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, long & rValue )
{
	rValue = [(id)object.get() longValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, unsigned long & rValue )
{
	rValue = [(id)object.get() unsignedLongValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, long long & rValue )
{
	rValue = [(id)object.get() longLongValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, unsigned long long & rValue )
{
	rValue = [(id)object.get() unsignedLongLongValue];
	
	return true;
}

bool convertScriptObjectTo( ScriptObject object, std::string & rValue )
{
	rValue = [(id)object.get() UTF8String];

	return true;
}

bool convertScriptObjectTo( ScriptObject object, Vector2 & rValue )
{
	if (!ScriptSequence::check( object ))
	{
		return false;
	}

	ScriptSequence seq( object );

	if (seq.size() != 2)
	{
		return false;
	}

	return seq.getItem( 0 ).convertTo( rValue[0] ) &&
		seq.getItem( 1 ).convertTo( rValue[1] );
}

bool convertScriptObjectTo( ScriptObject object, Vector3 & rValue )
{
	if (!ScriptSequence::check( object ))
	{
		return false;
	}
	
	ScriptSequence seq( object );
	
	if (seq.size() != 3)
	{
		return false;
	}
	
	return seq.getItem( 0 ).convertTo( rValue[0] ) &&
		seq.getItem( 1 ).convertTo( rValue[1] ) &&
		seq.getItem( 2 ).convertTo( rValue[2] );
}

bool convertScriptObjectTo( ScriptObject object, Vector4 & rValue )
{
	if (!ScriptSequence::check( object ))
	{
		return false;
	}
	
	ScriptSequence seq( object );
	
	if (seq.size() != 4)
	{
		return false;
	}
	
	return seq.getItem( 0 ).convertTo( rValue[0] ) &&
	seq.getItem( 1 ).convertTo( rValue[1] ) &&
	seq.getItem( 2 ).convertTo( rValue[2] ) &&
	seq.getItem( 3 ).convertTo( rValue[3] );
}

#endif // SCRIPT_OBJECTIVE_C
// oc_script_object.cpp
