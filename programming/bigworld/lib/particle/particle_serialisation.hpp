#ifndef PARTICLE_SERIALISATION_HPP
#define PARTICLE_SERIALISATION_HPP

BW_BEGIN_NAMESPACE
typedef SmartPointer< class DataSection > DataSectionPtr;
BW_END_NAMESPACE

/**
 * These helper structs assist with loading, saving and cloning particle
 * systems. They are based on the functionality of the SERIALISE,
 * SERIALISE_ENUM and SERIALISE_FUNC macros defined in datasection.hpp. The
 * main advantage of these helpers is they can also be used to clone an object
 * without going through serialisation.
 *
 * The datasection.hpp SERIALISE* macros use the property or member function
 * name as the tag to use for serialisation. This means renaming a variable or
 * function results in a different tag and existing data doesn't load
 * correctly. Because of this the tag can be manually specified to match it's
 * former value used by the datasection.hpp SERIALISE* macros.
 * 
 * The helpers also rely on there being getter and setter methods for all
 * members requiring serialisation. This was a pre-existing pattern that made
 * it easier to write the helpers.
 */
namespace BW
{

/// Helper writes Object values to DataSection using setter and getter member
/// function pointers.
template < typename ObjectType >
struct SaveToDataSection
{
	SaveToDataSection( DataSectionPtr & ds, const ObjectType * src ) :
		ds_( ds.get() ),
		src_( src )
	{}

	/// Casts enum value returned by getter to int and writes to DataSection
	template < typename GetType, typename SetType >
	void serialiseEnum( const StringRef & tag,
		GetType (ObjectType::*getter)() const,
		void (ObjectType::*)(SetType) ) const
	{
		ds_->write< int >( tag, (src_->*getter)() );
	}

	/// Writes value returned by getter to DataSection
	template < typename GetType, typename SetType >
	void serialise( const StringRef & tag,
		GetType (ObjectType::*getter)() const,
		void (ObjectType::*)(SetType) ) const
	{
		ds_->write( tag, (src_->*getter)() );
	}

	DataSection * ds_;
	const ObjectType * src_;
};

/// Helper sets Object values read from DataSection using setter member
/// function pointer.
template < typename ObjectType >
struct LoadFromDataSection
{
	LoadFromDataSection( DataSectionPtr & ds, ObjectType * dst ) :
		ds_( ds.get() ),
		dst_( dst )
	{}

	/// Sets enum value read from DataSection as int. Uses value returned
	/// by getter as a default value.
	template < typename GetType, typename SetType >
	void serialiseEnum( const StringRef & tag,
		GetType (ObjectType::*getter)() const,
		void (ObjectType::*setter)(SetType) ) const
	{
		(dst_->*setter)( static_cast< SetType >(
			ds_->read< int >( tag, (dst_->*getter)() ) ) );
	}

	/// Sets value read from DataSection on dst_. Uses value returned
	/// by getter as a default value.
	template < typename GetType, typename SetType >
	void serialise( const StringRef & tag,
		GetType ( ObjectType::*getter )() const,
		void (ObjectType::*setter)(SetType) ) const
	{
		(dst_->*setter)( ds_->read( tag, (dst_->*getter)() ) );
	}

	DataSection * ds_;
	ObjectType * dst_;
};

/// Helper writes Object values from source to dest using setter and getter
/// member function pointers
template < typename ObjectType >
struct CloneObject
{
	CloneObject( const ObjectType * src, ObjectType * dst ) :
		src_( src ),
		dst_( dst )
	{}

	/// Sets value on dst_ with value from src_ getter
	template < typename GetType, typename SetType >
	void serialiseEnum( const StringRef &,
		GetType ( ObjectType::*getter )() const,
		void (ObjectType::*setter)(SetType) ) const
	{
		(dst_->*setter)( (src_->*getter)() );
	}

	/// Sets value on dst_ with value from src_ getter
	template < typename GetType, typename SetType >
	void serialise( const StringRef &,
		GetType ( ObjectType::*getter )() const,
		void (ObjectType::*setter)(SetType) ) const
	{
		(dst_->*setter)( (src_->*getter)() );
	}

	const ObjectType * src_;
	ObjectType * dst_;
};

}

/// Serialise a value with tag generated from accessor name with trailing '_'.
#define BW_PARITCLE_SERIALISE( serialiser, type, accessor ) \
	serialiser.serialise( #accessor "_", &type::accessor, &type::accessor )
/// Serialise an enum with tag generated from accessor name with trailing '_'.
#define BW_PARITCLE_SERIALISE_ENUM( serialiser, type, accessor ) \
	serialiser.serialiseEnum( #accessor "_", &type::accessor, &type::accessor )

/// Serialise a value with the provided tag.
#define BW_PARITCLE_SERIALISE_NAMED( serialiser, type, accessor, tag ) \
	serialiser.serialise( tag, &type::accessor, &type::accessor )
/// Serialise an enum with the provided tag.
#define BW_PARITCLE_SERIALISE_ENUM_NAMED( serialiser, type, accessor, tag ) \
	serialiser.serialiseEnum( tag, &type::accessor, &type::accessor )

#endif // PARTICLE_SERIALISATION_HPP

