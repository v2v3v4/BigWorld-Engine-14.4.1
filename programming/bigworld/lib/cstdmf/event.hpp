#ifndef BW_EVENT_HPP
#define BW_EVENT_HPP

#include "stdmf.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE


/**
 * Basic Event type to support an event that multicasts to handlers
 * that take a Sender* and an EventArgs type as parameters. This class
 * was written as simply as possible to have low maintenance of 
 * templates and macros.
 * In future we would like to see this class reimplemented using C++11 and
 * variadic templates.
 *
 * Handler functions must have the following signature:
 *  void (XXXX::) ( Sender* pSender, EventArgs args )
 *
 * If you would like to avoid a copy of your arguments, be sure to define
 * const EventArgs& as your EventArgs type.
 */
template <typename Sender, typename EventArgs>
class Event
{
private:
	/**
	 * Traits class to help with definition of a member function pointer
	 * that occurs frequently in the rest of the class.
	 */
	template <typename TargetType>
	struct TargetTraits
	{
		typedef void(TargetType::*MemberFunctionPtr)( Sender *, EventArgs );
	};

	/**
	 * A basic and naive implementation of a C++ delegate based upon the 
	 * initial implementation from here: 
	 * http://www.codeproject.com/Articles/13287/Fast-C-Delegate
	 * Features of this particular delegate include:
	 *  - equality operator (not available in std::function)
	 *  - no heap allocations required (also not available in std::function)
	 * Unfortunately the nature of this delegate is such that it can only
	 * accept an explicit set of arguments unless we implement it using 
	 * variadic templates. Currently and in the past, this has been enough
	 * though.
	 */
	class EventDelegate
	{
	public:
		EventDelegate()
			: pTarget_( NULL )
			, pStubMethod_( NULL )
		{}


		/**
		 * Static templated helper function to generate the appropriate
		 * delegate given the target type and function pointer.
		 */
		template < class TargetType, 
			typename TargetTraits<TargetType>::MemberFunctionPtr MethodPtr>
		static EventDelegate fromMethod( TargetType * pTarget )
		{
			EventDelegate d;
			d.pTarget_ = pTarget;
			d.pStubMethod_ = &stub_method< TargetType, MethodPtr >::stub;

			return d;
		}


		/**
		 * Invocation operator to trigger the delegate.
		 */
		void operator()( Sender * pSender, EventArgs args ) const
		{
			return (*pStubMethod_)( pTarget_, pSender, args );
		}


		/**
		 * Equality operator to facilitate removal of registered 
		 * handlers.
		 */
		bool operator==( const EventDelegate & other )
		{
			return other.pStubMethod_ == pStubMethod_ &&
				other.pTarget_ == pTarget_;
		}

	private:
		typedef void (*StubMethodType)( void *, Sender *, EventArgs );

		void * pTarget_;
		StubMethodType pStubMethod_;

		template < class TargetType, 
			typename TargetTraits<TargetType>::MemberFunctionPtr MethodPtr >
		struct stub_method
		{
			static void stub( void * pVoidTarget, 
				Sender * pSender, EventArgs args )
			{
				TargetType * pTarget = 
					static_cast< TargetType * >(pVoidTarget);
				return (pTarget->*MethodPtr)( pSender, args );

			}
		};
	};

public:
	
	/**
	 * The list of delegates. Helper functions allow member function delegates
	 * to be specified without any heap allocations for each one.
	 */
	class EventDelegateList
	{
	public:
		

		template <typename TargetType, 
			typename TargetTraits<TargetType>::MemberFunctionPtr pMemberFunc>
			void add( TargetType * pTarget )
		{
			// If this assertion is hit, then the event list
			// is currently being iterated and is not safe 
			MF_ASSERT( !isIterating_ );

			EventDelegate del = 
				EventDelegate::template fromMethod<TargetType, pMemberFunc>( pTarget );
			delegates_.push_back( del );
		}

		template <typename TargetType,
			typename TargetTraits<TargetType>::MemberFunctionPtr pMemberFunc>
			void remove( TargetType * pTarget )
		{
			// If this assertion is hit, then the event list
			// is currently being iterated and is not safe 
			MF_ASSERT( !isIterating_ );

			EventDelegate del = 
				EventDelegate::template fromMethod<TargetType, pMemberFunc>( pTarget );
			delegates_.erase( 
				std::remove(
					delegates_.begin(), 
					delegates_.end(), 
					del ), 
				delegates_.end() );
		}

	private:
		friend Event;

		EventDelegateList() :
			isIterating_( false )
		{

		}

		void beginIteration() const
		{
			isIterating_ = true;
		}

		void endIteration() const
		{
			isIterating_ = false;
		}

		typedef BW::vector<EventDelegate> DelegateList;
		DelegateList delegates_;
		mutable bool isIterating_;
	};

	/**
	 * Invoke all event handlers passing the sender and the argument
	 * to all handlers.
	 * NOTE: At this point it is not supported to modify the contents of 
	 * the delegate list during invocation. An assertion will be raised
	 * if this is attempted.
	 */
	void invoke( Sender * pSender, EventArgs args ) const
	{
		list_.beginIteration();

		for (auto iter = list_.delegates_.begin(); 
			iter != list_.delegates_.end(); ++iter)
		{
			const EventDelegate & target = *iter;
			target( pSender, args );
		}

		list_.endIteration();
	}

	/**
	 * Expose the list of delegates to the owner of the event. The owner
	 * may then choose to expose this list to its public interface. This
	 * allows public add/remove of delegates, but only the owner may actually
	 * call 'invoke'.
	 */
	EventDelegateList & delegates()
	{
		return list_;
	}

private:
	EventDelegateList list_;
};


BW_END_NAMESPACE

#endif // BW_EVENT_HPP
