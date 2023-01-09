#ifndef METHOD_ARGS_HPP
#define METHOD_ARGS_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/smartpointer.hpp"

#include "bwentity/bwentity_api.hpp"
#include "network/basictypes.hpp"
#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;
class DataSection;
class DataType;
class DataSource;
class DataSink;
class MD5;

typedef SmartPointer< DataType > DataTypePtr;
typedef ConstSmartPointer< DataType > ConstDataTypePtr;
typedef SmartPointer< DataSection > DataSectionPtr;


/**
 *	This class stores the types of the method arguments and return values for
 *	MethodDescription instances.
 */
class MethodArgs
{
public:
	MethodArgs();

	bool parse( DataSectionPtr pSection, bool isOldStyle = false );

	bool checkValid( ScriptTuple args, const char * name,
			int firstOrdinaryArg = 0 ) const;

	BWENTITY_API
	bool addToStream( DataSource & source, BinaryOStream & stream ) const;
	BWENTITY_API bool createFromStream( BinaryIStream & data, DataSink & sink,
			const BW::string & name, EntityID * pImplicitSource = NULL ) const;

	void addToMD5( MD5 & md5 ) const;

	ScriptObject convertKeywordArgs( ScriptTuple args,
		ScriptDict kwargs ) const;
	ScriptDict createDictFromTuple( ScriptTuple args ) const;

	BWENTITY_API int clientSafety() const;

	ScriptObject typesAsScript() const;

	BWENTITY_API int size() const	{ return int( args_.size() ); }

	BWENTITY_API const DataType * operator[] ( int index ) const
	{
		return args_[ index ].second.get();
	}

	BWENTITY_API const BW::string & argName( int index ) const
	{
		return args_[ index ].first;
	}

	// Returns -1 if this is variable
	BWENTITY_API int streamSize() const { return streamSize_; }

	friend BWENTITY_API
	bool operator== ( const MethodArgs & left, const MethodArgs & right );

private:
	// TODO: This should be a ConstDataTypePtr, but DataType::isSameType is
	// non-const
	typedef BW::vector< std::pair< BW::string, DataTypePtr > > Args;
	Args args_;
	int streamSize_;
	
	// NOTE: If adding data, check the comparison operator.
};


BWENTITY_API
bool operator== ( const MethodArgs & left, const MethodArgs & right );

BWENTITY_API
bool operator!= ( const MethodArgs & left, const MethodArgs & right );


BW_END_NAMESPACE

#endif // METHOD_ARGS_HPP
