#ifndef PROPERTY_CHANGE_READER_HPP
#define PROPERTY_CHANGE_READER_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BitReader;
class PropertyOwnerBase;

typedef uint8 PropertyChangeType;


/**
 *	This base class is used to read in and apply a property change from a
 *	stream.
 */
class PropertyChangeReader
{
public:
	bool readSimplePathAndApply( BinaryIStream & stream,
			PropertyOwnerBase * pOwner,
			ScriptObject * ppOldValue,
			ScriptList * ppChangePath );

	int readCompressedPathAndApply( BinaryIStream & stream,
			PropertyOwnerBase * pOwner,
			ScriptObject * ppOldValue,
			ScriptList * ppChangePath );

protected:
	void doApply( BinaryIStream & stream,
		PropertyOwnerBase * pOwner, ScriptObject * ppOldValue,
		ScriptList * ppChangePath );

	virtual ScriptObject apply( BinaryIStream & stream,
			PropertyOwnerBase * pOwner, ScriptList pChangePath ) = 0;

	// Virtual methods to allow derived classes to read their specific
	// information
	virtual int readExtraBits( BinaryIStream & stream ) = 0;
	virtual int readExtraBits( BitReader & reader, int leafSize ) = 0;

	void updatePath( ScriptList * ppChangePath,
		ScriptObject pIndex = ScriptObject() ) const;
};


/**
 *	This class is used to read in and apply a change to a single property. It
 *	may be a simple property of an entity or a single value in another
 *	PropertyOwner like an array.
 */
class SinglePropertyChangeReader : public PropertyChangeReader
{
private:
	virtual ScriptObject apply( BinaryIStream & stream,
			PropertyOwnerBase * pOwner, ScriptList pChangePath );

	virtual int readExtraBits( BinaryIStream & stream );
	virtual int readExtraBits( BitReader & reader, int leafSize );

	int leafIndex_;
};


/**
 *	This class is used to read in and apply a change to an array that is not a
 *	single property. This includes adding and deleting elements and replacing
 *	slices.
 */
class SlicePropertyChangeReader : public PropertyChangeReader
{
private:
	virtual ScriptObject apply( BinaryIStream & stream,
			PropertyOwnerBase * pOwner, ScriptList pChangePath );

	virtual int readExtraBits( BinaryIStream & stream );
	virtual int readExtraBits( BitReader & reader, int leafSize );

	int32 startIndex_; //< The start index of the slice to replace
	int32 endIndex_; //< One after the end index of the slice to replace
};

BW_END_NAMESPACE

#endif // PROPERTY_CHANGE_READER_HPP
