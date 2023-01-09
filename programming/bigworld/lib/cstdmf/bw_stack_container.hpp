#ifndef BW_STACK_CONTAINER_HPP
#define BW_STACK_CONTAINER_HPP

#include "cstdmf/bw_stack_allocator.hpp"
#include "cstdmf/bw_string.hpp"

namespace BW
{
	/**
	 *
	 * Wraps an STL container  with the appropriate stack allocator
	 * 
	 */
	template<typename TContainerType, int CAPACITY>
	class StackContainer 
	{
	public:
		typedef TContainerType ContainerType;
		typedef typename ContainerType::value_type ContainedType;
		typedef BW::StackAllocator< ContainedType, CAPACITY> Allocator;

		StackContainer()
#ifdef ALLOW_STACK_CONTAINER
			: allocator_(&stack_data_)
			, stlAllocator_( &allocator_ )
			, container_(stlAllocator_)
#endif
		{
			//Attempt to reserve all the stack memory required 
			//immediately
			container_.reserve( CAPACITY );
#ifdef ALLOW_STACK_CONTAINER
			allocator_.setCanAllocate( false );
#endif
		}

		ContainerType& container() { return container_; }
		const ContainerType& container() const { return container_; }

		ContainerType* operator->() { return &container_; }
		const ContainerType* operator->() const { return &container_; }

	protected:
#ifdef ALLOW_STACK_CONTAINER
		typename Allocator::SourceBuffer stack_data_;
		Allocator allocator_;
		StlAllocator< ContainedType > stlAllocator_;
#endif
		ContainerType container_;
	};

	template<size_t CAPACITY>
	class StackString
		: public StackContainer< BW::string, CAPACITY >
	{
	public:
		StackString()
			: StackContainer< BW::string, CAPACITY >()
		{
		}
	};

	template<size_t CAPACITY>
	class WStackString
		: public StackContainer< BW::wstring, CAPACITY >
	{
	public:
		WStackString()
			: StackContainer< BW::wstring, CAPACITY >()
		{
		}
	};
}

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#define DECLARE_STACKSTR( name, CAPACITY )\
	StackString< CAPACITY > TOKENPASTE2( stackStr_##name##_, __LINE__);\
	BW::string & name = TOKENPASTE2( stackStr_##name##_, __LINE__ ).container();\

#define DECLARE_WSTACKSTR( name, CAPACITY )\
	WStackString< CAPACITY > TOKENPASTE2( stackStr_##name##_, __LINE__);\
	BW::wstring & name = TOKENPASTE2( stackStr_##name##_, __LINE__).container();\

#endif //BW_STACK_CONTAINER_HPP

