#include "dependency.hpp"

BW_BEGIN_NAMESPACE

namespace Dependency_Locals
{
	// XML tag names
	const char * CRITICAL_TAG = "Critical";
}

bool Dependency::serialiseIn( DataSectionPtr pSection )
{
	// Search for the critical tag
	DataSectionPtr criticalPtr = pSection->findChild( Dependency_Locals::CRITICAL_TAG );
	if (!criticalPtr || !criticalPtr->isAttribute())
	{
		// Could not find the critical tag,
		// there is an error with this data section
		return false;
	}

	// Parse the critical section
	critical_ = criticalPtr->asBool();
	return true;
}

bool Dependency::serialiseOut( DataSectionPtr pSection ) const
{
	// Create a critical sub section in the provided section 
	// and save out our critical flag.
	DataSectionPtr criticalPtr = pSection->newSection( Dependency_Locals::CRITICAL_TAG );
	MF_ASSERT( criticalPtr.hasObject() );
	MF_VERIFY( criticalPtr->isAttribute( true ) );
	MF_VERIFY( criticalPtr->setBool( critical_ ) );
	return true;
}

BW_END_NAMESPACE