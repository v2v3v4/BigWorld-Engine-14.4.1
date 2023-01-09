#include "script/first_include.hpp"

#include "py_cell_data.hpp"
#include "py_cell_spatial_data.hpp"
#include "server/cell_properties_names.hpp"
#include "cstdmf/memory_stream.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// No longer exposed so do not need to document.
/*  class NoModule.PyCellData
 *  @components{ base }
 *  An instance of PyCellData emulates a dictionary. It saves having to create
 *	properties from a stream until it is necessary.
 */
PY_TYPEOBJECT( PyCellData )

PY_BEGIN_METHODS( PyCellData )

	// Documented?
	/* function PyCellData getDict
	 *  @components{ base }
	 *  This method returns the cell properties as a dictionary.
	 *  @return The cell properties as a dictionary.
	 */
	PY_METHOD( getDict )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyCellData )
PY_END_ATTRIBUTES()


/**
 *	The constructor for PyCellData.
 */
PyCellData::PyCellData( EntityTypePtr pEntityType, BinaryIStream & data,
		bool persistentDataOnly, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pEntityType_( pEntityType ),
	pDict_( NULL ),
	data_(),
	dataHasPersistentOnly_( persistentDataOnly )
{
	int length = data.remainingLength();
	data_.assign( (char *)data.retrieve( length ), length );

	if (length == 0)
	{
		ERROR_MSG( "PyCellData::PyCellData: data is empty\n" );
	}
}


namespace
{
SpaceID getSpaceIDFromMap( const ScriptMapping & map )
{
	SpaceID spaceID = NULL_SPACE_ID;

	if (map)
	{
		ScriptObject id = map.getItem( SPACE_ID_PROPERTY_NAME, 
									    ScriptErrorClear() );
		if (id)
		{
			if (!id.convertTo( spaceID, ScriptErrorPrint() ))
			{
				WARNING_MSG( "PyCellData::getSpaceIDFromDict: "
						"Invalid property for spaceID\n" );
			}
		}
		else
		{
			WARNING_MSG( "PyCellData::getSpaceIDFromDict: No spaceID\n" );
		}
	}

	return spaceID;
}

Vector3 getVector3FromMap( const ScriptMapping & map, const char * pName )
{
	Vector3 v( 0.f, 0.f, 0.f );

	if (map)
	{
		ScriptObject val = map.getItem( pName, ScriptErrorClear() );

		if (val)
		{
			if (!val.convertTo( v, ScriptErrorPrint() ))
			{
				WARNING_MSG( "PyCellData::getVector3FromDict: "
						"Invalid property for %s\n", pName );
			}
		}
		else
		{
			WARNING_MSG( "PyCellData::getVector3FromDict: "
					"No item %s\n", pName );
		}
	}

	return v;
}
} // namespace


Vector3 PyCellData::getPosOrDir(
					PyObjectPtr pCellData, const char * pName, int offset )
{
	if (PyCellData::Check( pCellData.get() ))
	{
		PyCellData * pCD = static_cast<PyCellData *>( pCellData.get() );

		pCellData = pCD->pDict_;

		if (!pCellData)
		{
			return *(Vector3 *)(pCD->data_.data() + pCD->data_.size() - offset);
		}
	}

	if (ScriptMapping map = ScriptMapping::create( ScriptObject( pCellData ) ))
	{
		return getVector3FromMap( map, pName );
	}
	
	return Vector3();
}


/**
 *	This method returns the pos property associated with the input cell data.
 */
Vector3 PyCellData::getPos( PyObjectPtr pCellData )
{
	if (PyCellSpatialData::Check( pCellData.get() ))
	{
		return static_cast<PyCellSpatialData *>( pCellData.get() )->position();
	}
	
	return PyCellData::getPosOrDir( pCellData, POSITION_PROPERTY_NAME,
			2 * sizeof( Vector3 ) + sizeof( SpaceID ) );
}


/**
 *	This method returns the dir property associated with the input cell data.
 */
Direction3D PyCellData::getDir( PyObjectPtr pCellData )
{
	if (PyCellSpatialData::Check( pCellData.get() ))
	{
		return static_cast<PyCellSpatialData *>( pCellData.get() )->direction();
	}
	
	Vector3 dirVector = PyCellData::getPosOrDir( pCellData, 
			DIRECTION_PROPERTY_NAME, sizeof( Vector3 ) + sizeof( SpaceID ) );
	// XXX: This constructor takes its input as (roll, pitch, yaw)
	Direction3D result( dirVector );
	return result;
}


/**
 *	This method returns the spaceID property associated with the input cell
 *	data.
 */
SpaceID PyCellData::getSpaceID( PyObjectPtr pCellData )
{
	if (PyCellSpatialData::Check( pCellData.get() ))
	{
		return static_cast<PyCellSpatialData *>( pCellData.get() )->spaceID();
	}
	else if (PyCellData::Check( pCellData.get() ))
	{
		PyCellData * pCD = static_cast<PyCellData *>( pCellData.get() );

		pCellData = pCD->pDict_;

		if (!pCellData)
		{
			return *(SpaceID *)
				(pCD->data_.data() + pCD->data_.size() - sizeof(SpaceID));
		}
	}
	
	if (ScriptMapping map = ScriptMapping::create( ScriptObject( pCellData ) ))
	{
		return getSpaceIDFromMap( map );
	}
	
	return NULL_SPACE_ID;
}


bool PyCellData::getOnGround( PyObjectPtr pCellData )
{
	if (PyCellSpatialData::Check( pCellData.get() ))
	{
		return static_cast<PyCellSpatialData *>( pCellData.get() )->isOnGround();
	}
	else if (PyCellData::Check( pCellData.get() ))
	{
		pCellData = static_cast<PyCellData *>( pCellData.get() )->pDict_;
	}
	
	if (ScriptMapping map = ScriptMapping::create( ScriptObject( pCellData ) ))
	{
		ScriptObject isOnGround = 
			map.getItem( ON_GROUND_PROPERTY_NAME, ScriptErrorClear() );
		
		return isOnGround && PyObject_IsTrue(isOnGround.get()) == 1;
	}

	return false;
}


BW::string PyCellData::getTemplateID( PyObjectPtr pCellData )
{
	if (PyCellSpatialData::Check( pCellData.get() ))
	{
		return static_cast<PyCellSpatialData *>( pCellData.get() )->templateID();
	}
	else if (PyCellData::Check( pCellData.get() ))
	{
		pCellData = static_cast<PyCellData *>( pCellData.get() )->pDict_;
	}
	
	if (ScriptMapping map = ScriptMapping::create( ScriptObject( pCellData ) ))
	{
		ScriptBlob templateID = ScriptBlob::create(
				map.getItem( TEMPLATE_ID_PROPERTY_NAME, ScriptErrorClear() ));
		
		if (templateID)
		{
			BW::string strTemplateID;
			templateID.getString(strTemplateID);
			return strTemplateID;
		}
	}

	return BW::string();
}


void PyCellData::addSpatialData(  PyObjectPtr pCellData, const Vector3 & pos, 
								  const Direction3D & dir, SpaceID spaceID )
{
	PyObject * pPos = Script::getData( pos );
	PyObject * pDir = Script::getData( dir );
	PyObject * pSpaceID = Script::getData( spaceID );
	PyDict_SetItemString( pCellData.get(), POSITION_PROPERTY_NAME, pPos );
	PyDict_SetItemString( pCellData.get(), DIRECTION_PROPERTY_NAME, pDir );
	PyDict_SetItemString( pCellData.get(), SPACE_ID_PROPERTY_NAME, pSpaceID );
	Py_DECREF( pPos );
	Py_DECREF( pDir );
	Py_DECREF( pSpaceID );
}


/**
 *	This method adds this cell data to the input stream. If the dictionary was
 *	never created, it only needs to transfer the blob.
 */
bool PyCellData::addToStream( BinaryOStream & stream, bool addPosAndDir,
	bool addPersistentOnly )
{
	bool isOK;

	if (pDict_)
	{
		isOK = PyCellData::addMapToStream( stream, pEntityType_,
					ScriptMapping( pDict_ ), addPosAndDir, addPersistentOnly );
	}
	else
	{
		if (dataHasPersistentOnly_ == addPersistentOnly)
		{
			int size = data_.size();

			if (!addPosAndDir)
			{
				size -= 2 * sizeof( Vector3 ) + sizeof( SpaceID );
			}

			stream.addBlob( data_.data(), size );

			isOK = true;
		}
		else
		{
			// TODO: Can make this more efficient if we can copy directly from
			// data_ instead of creating a dictionary first.
			isOK = this->createPyDictOnDemand();
			if (isOK)
			{
				isOK = this->addMapToStream( stream, pEntityType_,
						ScriptMapping( pDict_ ), addPosAndDir, addPersistentOnly );
			}
		}
	}

	return isOK;
}

/**
 *	This static method adds cell properties as a dictionary into the stream.
 */
bool PyCellData::addMapToStream( BinaryOStream & stream,
	EntityTypePtr pEntityType, const ScriptMapping & map,
	bool addPosAndDir, bool addPersistentOnly )
{
	MF_ASSERT( map );

	int dataDomain = EntityDescription::CELL_DATA;
	if (addPersistentOnly)
		dataDomain |= EntityDescription::ONLY_PERSISTENT_DATA;
	
	if (pEntityType->description().addDictionaryToStream( map, stream, dataDomain ))
	{
		if (addPosAndDir)
		{
			stream <<	getVector3FromMap( map, POSITION_PROPERTY_NAME ) <<
						getVector3FromMap( map, DIRECTION_PROPERTY_NAME ) <<
						getSpaceIDFromMap( map );
		}

		return true;
	}

	return false;
}


/**
 *	This method returns the cell properties as a dictionary.
 */
PyObjectPtr PyCellData::getDict()
{
	if (!this->createPyDictOnDemand())
		PyErr_SetString( PyExc_ValueError,
						"Failed to create cellData from stream." );

	return pDict_;
}


/**
 *	This method creates the Python dictionary version of the cell properties
 * 	(as opposed to the original streamed version) when it is needed.
 */
PyObjectPtr PyCellData::createPyDictOnDemand()
{
	if (!pDict_)
	{
		if (!data_.empty())
		{
			MemoryIStream stream( (char *)data_.data(), data_.size() );
			pDict_ = pEntityType_->createCellDict( stream,
					dataHasPersistentOnly_ );

			if (pDict_)
				data_.clear();
		}
		else
		{
			ERROR_MSG( "PyCellData::createPyDictOnDemand: data_.size() = 0\n" );
		}
	}

	return pDict_;
}


/**
 *	This static method adds a PyCellData or dictionary to the input stream.
 *
 *	@param stream		The stream to add to.
 *	@param pType		A pointer to the entity type.
 *	@param pCellData	The Python object containing the properties.
 *	@param addPosAndDir	Indicates whether or not position and direction should
 *					be added to the stream.
 *	@param addPersistentOnly	Indicates whether or not non-persistent data should
 * 					be added to the stream.
 */
bool PyCellData::addToStream( BinaryOStream & stream, EntityTypePtr pType,
		PyObject * pCellData, bool addPosAndDir, bool addPersistentOnly )
{
	if (PyCellData::Check( pCellData ))
	{
		return ((PyCellData *)pCellData)->addToStream( stream, addPosAndDir,
												addPersistentOnly );
	}
	else if (ScriptMapping map = ScriptMapping::create( ScriptObject( pCellData ) ))
	{
		return PyCellData::addMapToStream( stream, pType, 
				map, addPosAndDir, addPersistentOnly );
	}

	return false;
}


/**
 *	This method migrates this cell data to the new given type.
 */
void PyCellData::migrate( EntityTypePtr pType )
{
	// turn it into a dict using the old type
	this->createPyDictOnDemand();

	// set in the new type
	pEntityType_ = pType;
}

BW_END_NAMESPACE

// py_cell_data.cpp
