#import <Foundation/Foundation.h>

#include "entitydef/method_description.hpp"
#include "entitydef/data_type.hpp"

#import <string>

bool MethodDescription::areValidArgs( bool exposedExplicit, ScriptObject args,
	bool generateException ) const
{
	return true;
}


/**
 *	This method adds a method to the input stream. It assumes that the
 *	arguments are valid, according to 'areValidArgs'. The method header
 *	will already have been put onto the stream by the time this is called,
 *	so we have to complete the message even if we put on garbage.
 *	@see areValidArgs
 *
 *	@param isFromServer Indicates whether or not we are being called in a
 * 					server component.
 *	@param args		A Python tuple object containing the arguments.
 *	@param stream	The stream to which the method call should be added.
 *
 *	@return	True if successful.
 */
bool MethodDescription::addToStream( bool isFromServer,
	ScriptObject args, BinaryOStream & stream ) const
{
	NSInvocation * invocation = (NSInvocation *)args.get();
	
	int arg = 2;

	int size = args_.size();
	
	for (int i = 0; i < size; ++i)
	{
		DataType * pType = args_[i];
		id idVal;
		[invocation getArgument:&idVal atIndex:arg];

		pType->addToStream( idVal, stream, false );
		
		++arg;
	}

	return true;
}


/**
 *	This method calls the method described by this object.
 *
 *	@param self the object whose method is called
 *	@param data the incoming data stream
 *	@param sourceID the source object ID if it came from a client
 *	@param replyID the reply ID, -1 if none
 *	@param pReplyAddr the reply address or NULL if none
 *	@param pInterface the nub to use, or NULL if none
 *
 *	@return		True if successful, otherwise false.
 */
bool MethodDescription::callMethod( ScriptObject entity,
	BinaryIStream & data,
	EntityID sourceID,
	int replyID,
	const Mercury::Address* pReplyAddr,
	Mercury::NetworkInterface * pInterface ) const
{
	NSString * methodName = [NSString stringWithUTF8String:this->name().c_str()];
	
	uint numArgs = args_.size();

	for (uint i = 0; i < numArgs; ++i)
	{
		methodName = [methodName stringByAppendingString:@":"];
	}
	
	SEL sel = NSSelectorFromString( methodName );
	
	if (!sel)
	{
		return false;
	}

	id bwentity = (id)entity.get();

	id myClass = [bwentity class];

	NSMethodSignature * mySignature = [myClass instanceMethodSignatureForSelector:sel];

	if (!mySignature)
	{
		return false;
	}

	NSInvocation * myInvocation = [NSInvocation invocationWithMethodSignature:mySignature];
	[myInvocation setTarget:(id)entity.get()];
	[myInvocation setSelector:sel];

	int index = 2;
	int size = args_.size();

	for (int i = 0; i < size; ++i)
	{
		ScriptObject argObject = args_[i]->createFromStream( data, false );
		id arg = (id)argObject.get();
		[myInvocation setArgument:&arg atIndex:index];

		++index;
	}
	
	[myInvocation invoke];
	
	return true;
}


bool MethodDescription::isMethodHandledBy( const ScriptObject & classObject ) const
{
	// TODO:
	return true;
}

ScriptObject MethodDescription::methodSignature() const
{
	std::string prefix( @encode( id ) );
	prefix += @encode( id );
	prefix += @encode( SEL );
	std::string signatureFormat = prefix;

	int size = args_.size();

	for (int i = 0; i < size; ++i)
	{
		// signatureFormat += args_[i]->objCFormat();
		signatureFormat += std::string( @encode( id ) );
	}

	return [NSMethodSignature signatureWithObjCTypes:signatureFormat.c_str()];
}


/**
 *	This method creates a JSON object representing an exception object. It is
 *	made up of a string containing the error type, and an object containing
 *	the arguments.
 */
ScriptObject MethodDescription::createErrorObject( const char * excType,
												  ScriptObject args )
{
	NSDictionary * errorUserInfo = [NSDictionary dictionaryWithObjectsAndKeys: @"args", (NSObject *)args.get(), nil];
	NSError * error = [NSError errorWithDomain: @"MethodDescription" 
										  code: 0 
									  userInfo: errorUserInfo];
	return ScriptObject( error );
}


// method_description.mm
