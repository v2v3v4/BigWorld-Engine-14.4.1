#include "folder_guard.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

//==============================================================================
FolderSetter::FolderSetter()
{
	BW_GUARD;

	GetCurrentDirectory( ARRAY_SIZE( envFolder_ ), envFolder_ );
	GetCurrentDirectory( ARRAY_SIZE( curFolder_ ), curFolder_ );
}


//==============================================================================
void FolderSetter::enter()
{
	BW_GUARD;

	GetCurrentDirectory( ARRAY_SIZE( envFolder_ ), envFolder_ );
	SetCurrentDirectory( curFolder_ );
}


//==============================================================================
void FolderSetter::leave()
{
	BW_GUARD;

	GetCurrentDirectory( ARRAY_SIZE( curFolder_ ), curFolder_ );
	SetCurrentDirectory( envFolder_ );
}


//==============================================================================
FolderGuard::FolderGuard()
{
	BW_GUARD;

	setter_.enter();
}


//==============================================================================
FolderGuard::~FolderGuard()
{
	BW_GUARD;

	setter_.leave();
}

BW_END_NAMESPACE
