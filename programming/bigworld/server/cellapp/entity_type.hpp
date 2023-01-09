#ifndef ENTITY_TYPE_HPP
#define ENTITY_TYPE_HPP

#include <Python.h>

#include "cstdmf/bw_string.hpp"

#include "cstdmf/stringmap.hpp"

#include "entitydef/entity_description.hpp"
#include "entitydef/method_description.hpp"

#include "network/basictypes.hpp"

#include "server/entity_type_profiler.hpp"

BW_BEGIN_NAMESPACE

class RealEntity;
class Entity;
class BinaryIStream;
class BinaryOStream;

enum CreateRealInfo
{
	CREATE_REAL_FROM_INIT,
	CREATE_REAL_FROM_OFFLOAD,
	CREATE_REAL_FROM_RESTORE
};


typedef SmartPointer<Entity> EntityPtr;

class EntityType;
typedef SmartPointer<EntityType> EntityTypePtr;
typedef BW::vector< EntityTypePtr > EntityTypes;

/**
 *	This class is used to represent an entity type.
 */
class EntityType : public ReferenceCount
{
public:
	static const EntityTypeID INVALID_INDEX = -1;

	EntityType( const EntityDescription& entityDescription,
			PyTypeObject * pType );

	~EntityType();

	Entity * newEntity() const;

	const VolatileInfo & volatileInfo() const;

	DataDescription * description( const char * attr, 
								const char * component) const;
	const EntityDescription & description() const;

	uint propCountGhost() const						{ return propCountGhost_; }
	uint propCountGhostPlusReal() const				{ return propDescs_.size();}
	DataDescription * propIndex( int index ) const	{ return propDescs_[index];}

	uint propCountClientServer() const
	{
		return entityDescription_.clientServerPropertyCount();
	}

	PyTypeObject * pPyType() const 	{ return pPyType_; }
	void setPyType( PyTypeObject * pPyType );

	const char * name() const;

	bool canBeOnBase() const  { return entityDescription_.canBeOnBase(); }
	bool canBeOnCell() const  { return entityDescription_.canBeOnCell(); }

	bool expectsNearbyEntity() const { return expectsNearbyEntity_; }

	EntityTypeID typeID() const			{ return entityDescription_.index(); }
	EntityTypeID clientTypeID() const;

	void addDataToStream( Entity * pEntity,
			BinaryOStream & stream,
			EntityDataFlags dataType );

	EntityTypePtr old() const	{ return pOldSelf_; }
	void old( EntityTypePtr pOldType );

	// profiling
	const EntityTypeProfiler & profiler() const { return profiler_; }
	EntityTypeProfiler & profiler() { return profiler_; }
	static void tickProfilers( float & totalEntityLoad,
								float & totalAddedLoad );

	/// @name static methods
	//@{
	static bool init( bool isReload = false );
	static bool reloadScript( bool isRecover = false );
	static void migrate( bool isFullReload = true );
	static void cleanupAfterReload( bool isFullReload = true );

	static void clearStatics();

	static EntityTypePtr getType( EntityTypeID typeID );
	static EntityTypePtr getType( const char * className );
	static const EntityDescription * getDescription( EntityTypeID typeID );
	static EntityTypeID nameToIndex( const char * name );

	static EntityTypes& getTypes();
	//@}

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();

	class Stats
	{
	public:
		Stats() : detailedPositionDescription_( "detailedPosition" ) {}

		void countSentToOwnClient( int bytes )
							{ sentToOwnClient_.count( bytes ); }

		void countNonVolatileSentToOtherClients( int bytes ) 
							{ nonVolatileSentToOtherClients_.count( bytes ); }

		void countVolatileSentToOtherClients(int bytes) 
							{ volatileSentToOtherClients_.count( bytes ); }

		void countAddedToHistoryQueue( int bytes )
							{ addedToHistoryQueue_.count( bytes ); }

		void countSentToGhosts( int bytes ) { sentToGhosts_.count( bytes ); }
		void countSentToBase( int bytes ) { sentToBase_.count( bytes ); }

		MemberDescription & detailedPositionDescription()
		{
			return detailedPositionDescription_;
		}

		const MemberDescription & detailedPositionDescription() const
		{
			return detailedPositionDescription_;
		}

		EntityMemberStats & createEntityCounter()
		{
			return createEntity_;
		}
		EntityMemberStats & enterAoICounter()
		{
			return enterAoI_;
		}
		static WatcherPtr pWatcher();

	private:
		EntityMemberStats::Stat sentToOwnClient_;
		EntityMemberStats::Stat nonVolatileSentToOtherClients_;
		EntityMemberStats::Stat volatileSentToOtherClients_;
		EntityMemberStats::Stat addedToHistoryQueue_;
		EntityMemberStats::Stat sentToGhosts_;
		EntityMemberStats::Stat sentToBase_;

		MemberDescription detailedPositionDescription_;

		EntityMemberStats createEntity_;
		EntityMemberStats enterAoI_;
	};

	Stats & stats() { return stats_; }
	MemberDescription & detailedPositionDescription()
	{
		return stats_.detailedPositionDescription();
	}
	const MemberDescription & detailedPositionDescription() const
	{
		return stats_.detailedPositionDescription();
	}

private:
	Stats stats_;

#else
	MemberDescription & detailedPositionDescription()
	{
		return detailedPositionDescription_;
	}
	const MemberDescription & detailedPositionDescription() const
	{
		return detailedPositionDescription_;
	}

private:
	MemberDescription detailedPositionDescription_;

#endif

	EntityDescription entityDescription_;
	PyTypeObject * pPyType_;

	uint							propCountGhost_;
	BW::vector<DataDescription*>	propDescs_;

	EntityTypePtr	pOldSelf_;

	bool expectsNearbyEntity_;

	// profiling
	EntityTypeProfiler profiler_;

	static EntityTypes s_curTypes_, s_newTypes_;

	typedef StringHashMap< EntityTypeID > NameToIndexMap;
	static NameToIndexMap s_nameToIndexMap_;

	static PyObject * s_pInitModules_, * s_pNewModules_;
};

typedef SmartPointer<EntityType> EntityTypePtr;


#ifdef CODE_INLINE
#include "entity_type.ipp"
#endif

BW_END_NAMESPACE

#endif // ENTITY_TYPE_HPP
// entity_type.hpp
