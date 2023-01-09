#ifndef _ASTAR_HEADER
#define _ASTAR_HEADER

#include "cstdmf/debug.hpp"
#include "waypoint/chunk_waypoint.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class implements an A* search. It will search anything that
 *	can be described with the following interface:
 *
 *	typedef \<some suitable type\> adjacency_iterator;
 *
 *	int	compare(const State& other) const
 *	bool isGoal(const State& goal) const
 *	adjacency_iterator adjacenciesBegin() const
 *	adjacency_iterator adjacenciesEnd() const
 *	bool getAdjacency( adjacency_iterator i, State& adj, const State& goal,
		SearchData & searchData ) const
 *	float distanceFromParent() const
 *	float distanceToGoal(const State& goal) const
 *
 *	SearchData is user data which gets passed to the getAdjacency method.
 */
template<class State, class SearchData = void*> 
class AStar
{
public:
	typedef State TState;

	class StateScopedFreeList;

	AStar( StateScopedFreeList * );
	~AStar();

	bool				search(const State& start, const State& goal,
								float maxDistance );
	const State*		first();
	const State*		next();

	SearchData & searchData()
	{
		return searchData_;
	}

private:
	StateScopedFreeList * freeList_;

	/**
	 *	This structure represents an internal state of the search.
	 */
	struct IntState
	{
		State		ext;		///< User defined state
		float		g;			///< Cost to get here from start
		float		h;			///< Estimated cost from here to goal
		float 		f;			///< g + h
		IntState*	pParent;	///< The state before us in the search
		IntState*	pChild;		///< The state after us in the search
		IntState*	hashNext;
		IntState*	freeNext;

		IntState() :
			g(),
			h(),
			f(),
			pParent(),
			pChild(),
			hashNext( NULL ),
			freeNext( NULL )
		{}
	};

public:
	
	// Scoped freelist is public as it is owned externally from the AStar class
	class StateScopedFreeList
	{
	public:
		StateScopedFreeList() :
			freeHead_( NULL )
		{
		}
		~StateScopedFreeList()
		{
			IntState * head = freeHead_;
			while (head != NULL)
			{
				IntState * next = head->freeNext;
				delete head;
				head = next;
			}
		}
		IntState * alloc()
		{
			if (freeHead_ == NULL)
			{
				return new IntState;
			}
			IntState * head = freeHead_;
			freeHead_ = head->freeNext;
			new (head) IntState;
			return head;
		}

		void free( IntState * state )
		{
			if (state == NULL)
			{
				return;
			}
			state->~IntState();
			state->freeNext = freeHead_;
			freeHead_ = state;
		}

	private:
		IntState * freeHead_;
	};

private:

	class IntStateHash
	{
		static const size_t BIN_SIZE = 257;
		IntState* bin_[BIN_SIZE];
		StateScopedFreeList * freeList_;
	public:
		IntStateHash( StateScopedFreeList * freeList ) :
			freeList_( freeList )
		{
			for( size_t i = 0; i < BIN_SIZE; ++i )
				bin_[ i ] = 0;
		}
		unsigned int hash( IntState* state )
		{
			return (unsigned int)( state->ext.hash() ) % BIN_SIZE;
		}
		IntState* find( IntState* state )
		{
			IntState* head = bin_[ hash( state ) ];
			while( head != NULL )
			{
				if( head->ext.compare( state->ext ) == 0 )
					return head;
				head = head->hashNext;
			}
			return NULL;
		}
		void erase( IntState* state )
		{
			IntState* head = bin_[ hash( state ) ];
			MF_ASSERT( head );

			if( head->ext.compare( state->ext ) == 0 )
				bin_[ hash( state ) ] = head->hashNext;
			else
			{
				IntState* next = head->hashNext;
				while( next != NULL )
				{
					if( next->ext.compare( state->ext ) == 0 )
					{
						head->hashNext = next->hashNext;
						next->hashNext = NULL;
						break;
					}
					head = next;
					next = head->hashNext;
				}
			}
		}
		void insert( IntState* state )
		{
			if( find( state ) )
				return;
			state->hashNext = bin_[ hash( state ) ];
			bin_[ hash( state ) ] = state;
		}
		void reset()
		{
			for( size_t i = 0; i < BIN_SIZE; ++i )
			{
				IntState* item = bin_[ i ];
				while( item )
				{
					IntState* next = item->hashNext;
					freeList_->free( item );
					item = next;
				}
				bin_[ i ] = NULL;
			}
		}
		size_t size() const
		{
			size_t size = 0;
			for( size_t i = 0; i < BIN_SIZE; ++i )
			{
				IntState* item = bin_[ i ];
				while( item )
				{
					++size;
					item = item->hashNext;
				}
			}
			return size;
		}
	};
	/**
	 *	This object is used to perform a comparison between internal
	 *	states on the priority queue. We want the first element on
	 *	the queue to be the state with the smallest value of f.
	 */
	struct cmp_f
	{
		bool operator()(const IntState* p1, const IntState* p2) const
		{
			if (almostEqual( p1->f, p2->f ))
			{
				return p1->g > p2->g;
			}

			return p1->f < p2->f;
		};
	};

	/**
	 *	This object is used to perform a comparison between internal
	 *	states in the set. It returns true if p1 is less than p2.
	 */
	struct cmp_ext
	{
		bool operator()(const IntState* p1, const IntState* p2) const
		{
			return p1->ext.compare(p2->ext) < 0;
		};
	};

	typedef IntStateHash AStarSet;
	typedef BW::set<IntState*> AStarQueue;


	IntState* 	iter_;
	IntState* 	start_;
	AStarSet 	close_;
	AStarQueue 	open_;
	SearchData	searchData_;
};

#include "astar.ipp"

BW_END_NAMESPACE

#endif

