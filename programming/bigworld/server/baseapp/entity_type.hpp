#ifndef ENTITY_TYPE_HPP
#define ENTITY_TYPE_HPP

#include "Python.h"

#include "cstdmf/md5.hpp"
#include "cstdmf/stringmap.hpp"

#include "entitydef/entity_description.hpp"
#include "entitydef/method_description.hpp"

#include "network/basictypes.hpp"

#include "server/entity_type_profiler.hpp"


BW_BEGIN_NAMESPACE

class Base;
class EntityType;

typedef SmartPointer< Base > BasePtr;
typedef SmartPointer< EntityType > EntityTypePtr;


/**
 *	This class is the entity type of a base
 */
class EntityType : public ReferenceCount
{
public:
	static const EntityTypeID INVALID_INDEX = -1;

	EntityType( const EntityDescription & desc, PyTypeObject * pType,
		   bool isProxy );
	~EntityType();

	BasePtr create( EntityID id, DatabaseID dbID, BinaryIStream & data,
		bool hasPersistentDataOnly, const BW::string * pLogOnData = NULL );

	BasePtr create( EntityID id,
					const BW::string & templateID );

	Base * newEntityBase( EntityID id, DatabaseID dbID );

	PyObject * createScript( BinaryIStream & data );
	PyObjectPtr createCellDict( BinaryIStream & data,
								bool strmHasPersistentDataOnly );

	const EntityDescription & description() const
											{ return entityDescription_; }

	static EntityTypePtr getType( EntityTypeID typeID,
			bool shouldExcludeServices = false );
	static EntityTypePtr getType( const char * className,
			bool shouldExcludeServices = false );
	static EntityTypeID nameToIndex( const char * name );
	static const EntityDescription * getDescription( EntityTypeID typeID,
		bool shouldExcludeServices = false );


	static bool init( bool isReload, bool isServiceApp );
	static bool reloadScript( bool isRecover = false );
	static void migrate( bool isFullReload = true );
	static void cleanupAfterReload( bool isFullReload = true );

	static void clearStatics();

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
	// Functions to collect statistical information
	void updateBackupSize( const Base & instance, uint32 newSize );
	void updateArchiveSize( const Base & instance, uint32 newSize );
	void countNewInstance( const Base & instance );
	void forgetOldInstance( const Base & instance );

	uint32 averageBackupSize() const;
	uint32 averageArchiveSize() const;
	uint32 totalBackupSize() const;
	uint32 totalArchiveSize() const;
#endif

	const char * name() const	{ return entityDescription_.name().c_str(); }

	bool canBeOnBase() const	{ return entityDescription_.canBeOnBase(); }
	bool canBeOnCell() const	{ return entityDescription_.canBeOnCell(); }

	bool isProxy() const		{ return isProxy_; }

	PyObject * pClass() const		{ return (PyObject*)pClass_; }
	void setClass( PyTypeObject * pClass );

	EntityTypeID id() const	{ return entityDescription_.index(); }

	bool isService() const	{ return entityDescription_.isService(); }

	EntityTypePtr old() const	{ return pOldSelf_; }
	void old( EntityTypePtr pOldType );

	// profiling
	const EntityTypeProfiler & profiler() const { return profiler_; }
	EntityTypeProfiler & profiler() { return profiler_; }
	static void tickProfilers();

	static const MD5::Digest & digest() { return s_digest_; }

	static MD5 s_persistentPropertiesMD5_;

private:
	EntityDescription entityDescription_;
	PyTypeObject *	pClass_;

	EntityTypePtr	pOldSelf_;

	bool			isProxy_;

#if ENABLE_WATCHERS
	// statistics
	uint32 backupSize_;
	uint32 archiveSize_;
	uint32 numInstancesBackedUp_;
	uint32 numInstancesArchived_;
	uint32 numInstances_;
#endif

	// profiling
	EntityTypeProfiler profiler_;

	// static stuff
	static PyObject * s_pInitModules_, * s_pNewModules_;

	typedef BW::vector< EntityTypePtr > EntityTypes;
	static EntityTypes s_curTypes_, s_newTypes_;

	typedef StringHashMap< EntityTypeID > NameToIndexMap;
	static NameToIndexMap s_nameToIndexMap_;

	static MD5::Digest s_digest_;
};

BW_END_NAMESPACE

#endif // ENTITY_TYPE_HPP

