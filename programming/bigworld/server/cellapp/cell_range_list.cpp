/**
 * This file was created to isolate some functions which were being optimised in
 * a funny way, which caused isSorted to fail even though it was in order.
 * This file should be compiled with -O0 option instead of -03
 */
#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "cell_range_list.hpp"

#include "profile.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "cell_range_list.ipp"
#endif

/**
 *	Construtor.
 */
RangeList::RangeList() : first_( true ), last_( false )
{
	last_.insertBeforeX( &first_ );
	last_.insertBeforeZ( &first_ );
}


/**
 *	This method adds an element to the range list.
 */
void RangeList::add( RangeListNode * pNode )
{
	SCOPED_PROFILE( SHUFFLE_ENTITY_PROFILE );

	MF_ASSERT( pNode != NULL );

	first_.nextX()->insertBeforeX( pNode );
	first_.nextZ()->insertBeforeZ( pNode );
	pNode->shuffleXThenZ( -FLT_MAX, -FLT_MAX );
}


/**
 *	This method checks that the range list is sorted after a shuffle.
 */
bool RangeList::isSorted() const
{
	const RangeListNode * prev = &first_;
	const RangeListNode * iter = first_.nextX();

	while (iter != NULL)
	{
		float prevX = prev->x();
		float iterX = iter->x();
		bool cond = prevX > iterX;
		if ( cond )
		{
			DEBUG_MSG( "LIST NOT SORTED in X: offending entries - prev=[%s, %.20f, 0x%08x], this=[%s, %.20f, 0x%08x]\n",
						prev->debugString().c_str(),
						prevX,
						*(int*)&prevX,
						iter->debugString().c_str(),
						iterX,
						*(int*)&iterX
						);

			DEBUG_MSG("LIST NOT SORTED in X: cond%d =%d >%d <%d\n",
						cond,
						isEqual( prevX, iterX ),
						prevX > iterX,
						prevX < iterX
					);
			prev->debugRangeX();
			iter->debugRangeX();
			return false;
		}
		prev = iter;
		iter = iter->nextX();
	}

	prev = &first_;
	iter = first_.nextZ();
	while (iter != NULL)
	{
		float prevZ = prev->z();
		float iterZ = iter->z();
		bool cond = prevZ > iterZ;
		if ( cond )
		{
			DEBUG_MSG("LIST NOT SORTED in Z: offending entries - prev=[%s, %.20f, 0x%08x], this=[%s, %.20f, 0x%08x]\n",
						prev->debugString().c_str(),
						prevZ,
						*(int*)&prevZ,
						iter->debugString().c_str(),
						iterZ,
						*(int*)&iterZ
						);

			DEBUG_MSG("LIST NOT SORTED in Z: cond%d =%d >%d <%d\n",
						cond,
						isEqual( prevZ, iterZ ),
						prevZ > iterZ,
						prevZ < iterZ
					);
			prev->debugRangeZ();
			iter->debugRangeZ();
			return false;
		}
		prev = iter;
		iter = iter->nextZ();
	}
	return true;
}


/**
 *	Dumps out debug information.
 */
void RangeList::debugDump()
{
	DEBUG_MSG("RangeListX:\n");
	const RangeListNode * iter2 = &first_;
	while(iter2 != NULL)
	{
		iter2->debugRangeX();
		iter2 = iter2->nextX();
	}

	DEBUG_MSG("RangeListZ:\n");
	iter2 = &first_;
	while(iter2 != NULL)
	{
		iter2->debugRangeZ();
		iter2 = iter2->nextZ();
	}
}

BW_END_NAMESPACE

// cell_range_list.cpp
