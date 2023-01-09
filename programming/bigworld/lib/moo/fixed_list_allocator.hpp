#ifndef FIXED_LIST_ALLOCATOR_HPP
#define FIXED_LIST_ALLOCATOR_HPP

#include "cstdmf/guard.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	/**
	 *	A template class for allocation of objects maintained in a static
	 *	list per object type. The objects in the list are recycled each frame.
	 */
	template<typename ObjectType>
	class FixedListAllocator : public RenderContextCallback
	{
	public:
		FixedListAllocator() :
		  offset_( 0 )
		{
		}

		~FixedListAllocator()
		{
			BW_GUARD;
			// Make sure fini was called and no further allocations were made.
			MF_ASSERT_DEV( container_.empty() );
		}

		typedef SmartPointer<ObjectType> ObjectPtr;
		typedef BW::vector<ObjectPtr> Container;

		// TODO: Find a way to remove this singleton/static
		static FixedListAllocator& instance()
		{
			static FixedListAllocator s_instance;
			return s_instance;
		}

		ObjectType* alloc()
		{
			BW_GUARD;
			static uint32 timestamp = -1;
			if (!rc().frameDrawn(timestamp))
				offset_ = 0;
			if (offset_ >= container_.size())
			{
				container_.push_back( new ObjectType );
			}
			return container_[offset_++].getObject();
		}

		/*virtual*/ void renderContextFini()
		{
			container_.clear();
		}

	private:
		Container	container_;
		uint32		offset_;
	};
}

BW_END_NAMESPACE

#endif
