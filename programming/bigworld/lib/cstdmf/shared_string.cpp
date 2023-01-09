#include "pch.hpp"
#include "shared_string.hpp"
#include "concurrency.hpp"

BW_BEGIN_NAMESPACE

const BW::string SharedString::s_EmptyString;
SharedString::StringMapType SharedString::s_SharedStrings;
SimpleMutex SharedString::s_Mutex;
size_t SharedString::s_SavedMemory = 0;

//==============================================================================
SharedString::SharedString( const SharedString & other )
{
	SimpleMutexHolder mutexHolder( s_Mutex );
	MF_ASSERT( other.string_ );
	assign( *other.string_ );
}


//==============================================================================
SharedString::SharedString( const BW::string & string )
	: string_( NULL )
{
	SimpleMutexHolder mutexHolder( s_Mutex );
	assign( string );
}


//==============================================================================
SharedString::~SharedString()
{
	SimpleMutexHolder mutexHolder( s_Mutex );
	unassign();
}


//==============================================================================
const BW::string & SharedString::getString() const
{
	SimpleMutexHolder mutexHolder( s_Mutex );
	MF_ASSERT( string_ );
	return *string_;
}


//==============================================================================
const SharedString & SharedString::operator = ( const SharedString & other )
{
	if (&other == this)
	{
		//Protect against people doing silly things
		return *this;
	}

	SimpleMutexHolder mutexHolder( s_Mutex );

	if (string_ != other.string_)
	{
		MF_ASSERT( other.string_ );
		unassign();
		assign( *other.string_ );
	}

	return *this;
}


//==============================================================================
const SharedString & SharedString::operator = ( const BW::string & other )
{
	SimpleMutexHolder mutexHolder( s_Mutex );

	if (string_ != &other)
	{
		unassign();
		assign( other );
	}

	return *this;
}


//==============================================================================
void SharedString::unassign()
{
	StringMapType::iterator findIt = s_SharedStrings.find( *string_ );
	MF_ASSERT( findIt != s_SharedStrings.end() );
	findIt->second--;
	s_SavedMemory -= string_->capacity();
	string_ = NULL;
	if (findIt->second == 0)
	{
		s_SharedStrings.erase( findIt );
	}
}


//==============================================================================
void SharedString::assign( const BW::string & string )
{
	StringMapType::iterator findIt = s_SharedStrings.find( string );
	if (findIt == s_SharedStrings.end())
	{
		std::pair< StringMapType::iterator, bool > insertIt =
			s_SharedStrings.insert( std::make_pair( string, 1 ) );
		string_ = &insertIt.first->first;
	}
	else
	{
		string_ = &findIt->first;
		findIt->second++;
		s_SavedMemory += string_->capacity();
	}
}

BW_END_NAMESPACE