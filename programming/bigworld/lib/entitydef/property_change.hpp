#ifndef PROPERTY_CHANGE_HPP
#define PROPERTY_CHANGE_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

#include "script/script_object.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;
class BitWriter;
class DataType;
class PropertyOwnerBase;


// -----------------------------------------------------------------------------
// Section: PropertyChange
// -----------------------------------------------------------------------------

/**
 *	This class represents a change to a property of an entity.
 */
class PropertyChange
{
public:
	enum Flags
	{
		FLAG_IS_SLICE	= 1 << 0,
		FLAG_IS_NESTED	= 1 << 1
	};

	PropertyChange( const DataType & type );

	virtual void addToInternalStream( BinaryOStream & stream ) const;

	virtual void addToExternalStream( BinaryOStream & stream,
			int clientServerPropertyID,
			int numClientServerProperties ) const;

	virtual void addValueToStream( BinaryOStream & stream ) const = 0;

	virtual bool isSlice() const = 0;

	void rootIndex( int rootIndex ) { rootIndex_ = rootIndex; }
	int rootIndex() const
	{ return this->isNestedChange() ? rootIndex_ : this->getRootIndex(); }

	void addToPath( int index, int indexLength )
	{
		path_.push_back( ChangePath::value_type( index, indexLength ) );
	}

	void isNestedChange( bool value ) { isNestedChange_ = true; }
	bool isNestedChange() const { return isNestedChange_; }

protected:
	// A sequence of child indexes ordered from the leaf to the root
	// (i.e. entity). For example, 3,4,6 would be the 6th property of the
	// entity, the 4th "child" of that property and then the 3rd "child".
	// E.g. If the 6th property is a list of lists called myList, this refers
	// to entity.myList[4][3]
	typedef BW::vector< std::pair< int32, int32 > > ChangePath;

	void addInternalFlags( BinaryOStream & stream ) const;

	void addSimplePathToStream( BinaryOStream & stream ) const;
	void addCompressedPathToStream( BinaryOStream & stream,
			int clientServerID, int numClientServerProperties ) const;

	void writePathSimple( BinaryOStream & stream ) const;

	virtual void addExtraBits( BitWriter & writer ) const = 0;
	virtual void addExtraBits( BinaryOStream & stream ) const = 0;

	virtual int getRootIndex() const	{ return 0; }

	const DataType & type_; //< Type of the new value(s).
	ChangePath path_; //< Path to the owner being changed.
	bool isNestedChange_; //< True if this change is a nested change
	int rootIndex_;
};


/**
 *	This class is a specialised PropertyChange. It represents a single value of
 *	an entity changing.
 */
class SinglePropertyChange : public PropertyChange
{
public:
	SinglePropertyChange( int leafIndex, int leafSize, const DataType & type );

	virtual void addValueToStream( BinaryOStream & stream ) const;

	virtual bool isSlice() const { return false; }

	void setValue( ScriptObject pValue )	{ pValue_ = pValue; }

private:
	virtual void addExtraBits( BitWriter & writer ) const;
	virtual void addExtraBits( BinaryOStream & stream ) const;

	virtual int getRootIndex() const	{ return leafIndex_; }

	int leafIndex_;
	int leafSize_;
	ScriptObject pValue_;
};


/**
 *	This class is a specialised PropertyChange. It represents a change to an
 *	array replacing a slice of values.
 */
class SlicePropertyChange : public PropertyChange
{
public:
	SlicePropertyChange( size_t startIndex, size_t endIndex,
			size_t originalLeafSize,
			const BW::vector< ScriptObject > & newValues,
			const DataType & type );

	virtual void addValueToStream( BinaryOStream & stream ) const;

	virtual bool isSlice() const { return true; }

private:

	virtual void addExtraBits( BitWriter & writer ) const;
	virtual void addExtraBits( BinaryOStream & stream ) const;

	size_t startIndex_;
	size_t endIndex_;
	size_t originalLeafSize_;
	const BW::vector< ScriptObject > & newValues_;
};

BW_END_NAMESPACE

#endif // PROPERTY_CHANGE_HPP
