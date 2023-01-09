#ifndef TERRAIN_TERRAIN_LOD_MANAGER_HPP
#define TERRAIN_TERRAIN_LOD_MANAGER_HPP

#include "vertex_lod_entry.hpp"
#include "resource.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
	class TerrainBlock2;
	typedef SmartPointer<VertexLodEntry>			VertexLodEntryPtr;

	/**
	 * A reference countable array of VertexLodEntryPtr, to be used as the
	 * "object type" of the Resource<>.
	 */
	struct VertexLodArray : public BW::vector<VertexLodEntryPtr>,
							public SafeReferenceCount
	{
		VertexLodArray( uint32 size ) : BW::vector<VertexLodEntryPtr>( size )
		{
		}
	};

	/**
	*	This class manages the set of vertex LOD entries for a single terrain 
	*	block, generating or unloading as necessary.
	*
	* @see VertexLodEntry
	* @see TerrainBlock2
	*/
	class VertexLodManager : public Resource< VertexLodArray >
	{
	public:
		/**
		 * This is the range of LOD indexes (inclusive) that comprise a working
		 * set. A working set is always contiguous.
		 */
		struct WorkingSet
		{			
 			inline WorkingSet() : start_(0), end_(0)
 			{}

			// Returns true if a set is wholly contained by or equal to the 
			// other set.
			inline bool IsWithin( const WorkingSet & other)
			{
				return ( start_ >= other.start_ && start_ <= other.end_ &&
						 end_ >= other.start_	&& end_ <= other.end_ );
			}

			// Returns true if any part of this set overlaps the other set.
			inline bool IsOverlapping( const WorkingSet & other )
			{
				return ( (start_ > other.start_ && start_ < other.end_) ||
					     (end_   > other.start_ && end_   < other.end_) );
			}

			uint32 start_;
			uint32 end_;
		};

		VertexLodManager( TerrainBlock2& owner, uint32 numLods );
		~VertexLodManager();

		/**
		 *	Resource overrides
		 */
		inline void evaluate( uint32 lod, uint32 topLod );
		virtual void stream( ResourceStreamType streamType = defaultStreamType() );
		virtual bool load(); 

		const VertexLodEntryPtr& getLod( uint32 level, bool doSubstitution = false );
		inline void getCurrentWorkingSet( WorkingSet& workingSet ) const ;
		inline void getRequestedWorkingSet( WorkingSet& workingSet ) const;
		inline uint32 getNumLods() const;

		virtual void preAsyncLoad();
		virtual void postAsyncLoad();

		/**
		* Return desired size of vertex grid for a given lod value.
		* Note lod 0 is highest.
		*/
		static inline uint32 getLodSize( uint32 lodLevel, uint32 numLods )
		{
			return 1 << ( numLods - lodLevel );
		}

		uint32 getLowestLod() const;

	protected:
		void getTargetWorkingSet( uint32 lod, WorkingSet & workingSet, uint32 topLod ) const ;
		void evictNotInSet( const WorkingSet & workingSet, bool preserveOne );
		virtual bool generate( uint32 level );

		// Data members
	protected:
		// Owner object
		TerrainBlock2&		owner_;
		// What lods are currently loaded
		WorkingSet			currentWorkingSet_;
		// What lods are requested by user
		WorkingSet			requestedWorkingSet_;
		// What lods are being loaded right now
		WorkingSet			loadingWorkingSet_;

	private:
		// no copying
		VertexLodManager( const VertexLodManager& );
		VertexLodManager& operator=( const VertexLodManager& );
	};

	/**
	* This method returns the current working set.
	*/
	inline void VertexLodManager::getCurrentWorkingSet( WorkingSet& workingSet ) 
		const
	{
		workingSet = currentWorkingSet_;
	}

	/**
	 * This method returns the last requested working set.
	 */
	inline void VertexLodManager::getRequestedWorkingSet( WorkingSet& workingSet ) 
		const
	{
		workingSet = requestedWorkingSet_;
	}

	/**
	* Return total number of LODS that may be cached for this block.
	*/
	inline uint32 VertexLodManager::getNumLods() const 
	{ 
		size_t numLods = object_ ? object_->size() : 0; 
		MF_ASSERT( numLods <= UINT_MAX );
		return ( uint32 )numLods;
	}

	/**
	* Get range of terrain LODs that are required for a given LOD level.
	* This will correctly account for the end cases where there is only
	* one neighbour.
	*
	* @param lod			target LOD index.
	* @param workingSet	result set.
	* @param topLod the top lod level to use
	*/
	inline void VertexLodManager::getTargetWorkingSet(
												uint32		lod,
												WorkingSet& workingSet,
												uint32 topLod ) const
	{
		// start is one before or topLod
		workingSet.start_ = lod;
		if ( workingSet.start_ > topLod ) workingSet.start_--;

		// end is one after or max
		workingSet.end_ = lod;
		if ( workingSet.end_ < object_->size() - 1) workingSet.end_++;
	}

	inline void VertexLodManager::evaluate( uint32 lod, uint32 topLod )
	{
		MF_ASSERT_DEBUG( lod < getNumLods() && "Requested an invalid lod" );
		getTargetWorkingSet( lod, requestedWorkingSet_, topLod ); 
	}

} // namespace Terrain

BW_END_NAMESPACE

#endif // VERTEX_LOD_MANAGER_HPP
