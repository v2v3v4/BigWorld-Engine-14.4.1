/**
 *	@file
 */
 
// -----------------------------------------------------------------------------
// Section: ScopedBool
// -----------------------------------------------------------------------------

inline ScopedBool::ScopedBool( bool & target )
	: target_(target)
{
	target_ = true;
}


inline ScopedBool::~ScopedBool()
{
	target_ = false;
}
