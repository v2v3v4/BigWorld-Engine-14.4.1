#include "pch.hpp"
#include "worldeditor/editor/chunk_placer.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/items/editor_chunk_vlo.hpp"
#include "chunk/chunk_stationnode.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "gizmo/item_view.hpp"
#include "gizmo/tool_locator.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/unique_id.hpp"


DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkPlacer
// 
// This file implements the prefab related functionality of ChunkPlacer
//
// -----------------------------------------------------------------------------


PY_MODULE_STATIC_METHOD( ChunkPlacer, saveChunkPrefab, WorldEditor )
PY_MODULE_STATIC_METHOD( ChunkPlacer, loadChunkPrefab, WorldEditor )

/**
 *	Return the DataSectionPtr for the given filename, creating it's directory
 *	if needed.
 */
static DataSectionPtr openOrCreate( const BW::string& fileName )
{
	BW_GUARD;

	BW::string dirName = BWResource::getFilePath( fileName );

	DataSectionPtr dir = BWResource::openSection( dirName, true );
	if (!dir)
	{
		ERROR_MSG( "openOrCreate() - Couldn't open dir %s\n",
			dirName.c_str() );
		return NULL;
	}

	DataSectionPtr ds = dir->newSection( BWResource::getFilename( fileName ) );
	if (!ds)
	{
		ERROR_MSG( "openOrCreate() - Couldn't create file\n" );
		return NULL;
	}

	return ds;
}

/**
 *	Calculate world space average position of the given items
 */
static Vector3 placerPrefabCalculateAverageOrigin( const ChunkItemRevealer::ChunkItems& items )
{
	BW_GUARD;

	Vector3 averagePosition = Vector3::zero();
	for (ChunkItemRevealer::ChunkItems::const_iterator i = items.begin();
			i != items.end();
			++i )
	{
		Chunk* chunk = (*i)->chunk();

		if (!chunk->isBound())
			continue;

		averagePosition += chunk->transform().applyPoint(
			(*i)->edTransform().applyToOrigin() );
	}
	averagePosition /= (float) items.size();

	return averagePosition;
}

/**
 * Calculate a bounding box encompasing all the items
 */
static BoundingBox calculateBoundingBox( const ChunkItemRevealer::ChunkItems& items )
{
	BW_GUARD;

	BoundingBox bb = BoundingBox::s_insideOut_;

	for (ChunkItemRevealer::ChunkItems::const_iterator i = items.begin();
			i != items.end();
			++i )
	{
		ChunkItemPtr item = *i;

		if (item->isShellModel())
		{
			bb.addBounds( item->chunk()->boundingBox() );
		}
		else
		{
			BoundingBox itembb;
			item->edBounds( itembb );

			itembb.transformBy( item->edTransform() );
			itembb.transformBy( item->chunk()->transform() );

			bb.addBounds( itembb );
		}
	}

	return bb;
}

static void calculateSnaps( const ChunkItemRevealer::ChunkItems& items, Vector3& positionSnaps, float& angleSnaps )
{
	BW_GUARD;

	positionSnaps = Vector3::zero();
	angleSnaps = 0.f;

	for (ChunkItemRevealer::ChunkItems::const_iterator i = items.begin();
			i != items.end();
			++i )
	{
		angleSnaps = Snap::satisfy( angleSnaps, (*i)->edAngleSnaps() );
		Vector3 m = (*i)->edMovementDeltaSnaps();

		positionSnaps.x = Snap::satisfy( positionSnaps.x, m.x );
		positionSnaps.y = Snap::satisfy( positionSnaps.y, m.y );
		positionSnaps.z = Snap::satisfy( positionSnaps.z, m.z );
	}
}

static void removeRefVLOs( Chunk * pChunk, const DataSectionPtr & ds, BW::map< BW::string, ChunkItemPtr >& vlo )
{
	BW_GUARD;

	DataSectionPtr vloSec;
	while ( (vloSec = ds->findChild( "vlo" )) != NULL )
	{
		ds->delChild( "vlo" );
		BW::string id = vloSec->readString( "uid" );
		VeryLargeObjectPtr pExistingVlo = VeryLargeObject::getObject( UniqueID( id ) );
		if (pExistingVlo != NULL)
		{
			ChunkVLO * item = pExistingVlo->containsChunk( pChunk );
			if (item && item->edBelongToChunk())
			{
				vlo.insert( std::pair< BW::string, ChunkItemPtr >( id, item ) );
			}
		}
	}
}

static bool savePrefab( const BW::string& prefabName, ChunkItemRevealer::ChunkItems& items )
{
	BW_GUARD;

	// Create a new file in the models dir
	DataSectionPtr rootDS = openOrCreate( prefabName );
	if (!rootDS)
	{
		ERROR_MSG( "savePrefab() - Couldn't create file\n" );
		return false;
	}
	BW::string binFileName = BWResource::changeExtension( prefabName,
		".pdata" );
	DataSectionPtr binDS = new BinSection(
		BWResource::getFilename( binFileName ),
		new BinaryBlock( NULL, 0, "BinaryBlock/ChunkPlacerPrefab" ) );

	DataSectionPtr chunkDS = rootDS->newSection( "chunks" );
	DataSectionPtr itemDS = rootDS->newSection( "items" );
	if (!chunkDS || !itemDS)
	{
		ERROR_MSG( "savePrefab() - Couldn't create sub sections\n" );
		return false;
	}

	Vector3 positionSnaps;
	float angleSnaps;
	calculateSnaps( items, positionSnaps, angleSnaps );

	// Centre all the items to be around the origin
	Vector3 averageOrigin = placerPrefabCalculateAverageOrigin( items );
	Snap::vector3( averageOrigin, positionSnaps );
	Vector3 offsetToOrigin = averageOrigin * -1.f;

	int chunkNumber = 0;
	int itemNumber = 0;

	ChunkItemRevealer::ChunkItems dupItems(items.begin(), items.end());
	BW::map< BW::string, ChunkItemPtr > refVLOs;
	for (ChunkItemRevealer::ChunkItems::iterator i = dupItems.begin();
			i != dupItems.end();
			++i )
	{
		ChunkItemPtr item = *i;
		Chunk* chunk = item->chunk();

		if (!chunk->isBound())
		{
			WARNING_MSG( "Chunk isn't bound, can't save as prefab\n" );
			return false;
		}

		if (item->isShellModel())
		{
			// Write the entire chunk out
			char chunkNumberStr[10];
			bw_snprintf( chunkNumberStr, sizeof(chunkNumberStr), "%d", chunkNumber );
			BW::string chunkName( "chunk" + BW::string( chunkNumberStr ) + ".chunk" );
			DataSectionPtr pDS = chunkDS->newSection( chunkName );
			if (!pDS)
			{
				ERROR_MSG( "savePrefab() - Couldn't create new section\n" );
				continue;
			}

			ChunkPlacer::utilCloneChunkDataSection( chunk, pDS, chunkName );

			pDS->deleteSections( "navmesh" );

			// boof the positions of the slected chunks be around the origin
			Matrix chunkTransform = pDS->readMatrix34( "transform" );
			chunkTransform.postTranslateBy( offsetToOrigin );
			pDS->writeMatrix34( "transform", chunkTransform );

			// boof the bounding box too!
			Vector3 bbMin = pDS->readVector3( "boundingBox/min" );
			bbMin += offsetToOrigin;
			pDS->writeVector3( "boundingBox/min", bbMin );

			Vector3 bbMax = pDS->readVector3( "boundingBox/max" );
			bbMax += offsetToOrigin;
			pDS->writeVector3( "boundingBox/max", bbMax );


			chunkNumber++;
			if (chunkNumber >= 1000000)
			{
				// too many
				ERROR_MSG( "savePrefab(): Too many chunks in prefab, max is 1000000\n" );
				return false;
			}
			removeRefVLOs( chunk, pDS, refVLOs );

		}
	}
	for (ChunkItemRevealer::ChunkItems::iterator i = dupItems.begin();
			i != dupItems.end();
			++i )
	{
		ChunkItemPtr item = *i;
		Chunk* chunk = item->chunk();

		if (!item->isShellModel())
		{
			DataSectionPtr origDS = item->pOwnSect();
			if ( origDS->sectionName() == "vlo" )
			{
				refVLOs.erase( origDS->readString( "uid", "" ) );
			}
		}
	}
	for (BW::map< BW::string, ChunkItemPtr >::iterator i = refVLOs.begin(); i != refVLOs.end(); ++i)
	{
		dupItems.push_back( i->second );
	}

	refVLOs.clear();

	for (ChunkItemRevealer::ChunkItems::iterator i = dupItems.begin();
			i != dupItems.end();
			++i )
	{

		ChunkItemPtr item = *i;
		Chunk* chunk = item->chunk();

		if (!item->isShellModel())
		{
			// Write the item's datasection out
			DataSectionPtr origDS = item->pOwnSect();
			DataSectionPtr pDS = itemDS->newSection( origDS->sectionName() );
			if (!pDS)
			{
				ERROR_MSG( "savePrefab() - Couldn't create new section\n" );
				continue;
			}

			pDS->copy( origDS );

			if ( origDS->sectionName() == "vlo" )
			{
				pDS->delChild("uid");
				EditorChunkVLO* vloChunkItem = reinterpret_cast<EditorChunkVLO*>(item.getObject());
				if (vloChunkItem)
				{
					pDS->copySections( vloChunkItem->object()->section() );
					BW::string vloType = pDS->readString("type","");
					if (vloType != "")
					{
						DataSectionPtr vloDS = pDS->openSection(vloType);
						if (vloDS)
							vloDS->delChild("position");
					}
				}
			}

			// Add its location so we can place in back correctly
			Matrix m;
			m.multiply( item->edTransform(), chunk->transform() );
			m.postTranslateBy( offsetToOrigin );
			pDS->writeMatrix34( "prefabTransform", m );

			// Get the binary data for the item
			BinaryPtr bp = item->edExportBinaryData();
			if (bp)
			{
				char itemNumberStr[16];
				bw_snprintf( itemNumberStr, sizeof(itemNumberStr), "item%02d", itemNumber++ );

				binDS->writeBinary( itemNumberStr, bp );
				pDS->writeString( "prefabBin", itemNumberStr );
			}
		}

	}

	BoundingBox bb = calculateBoundingBox( items );

	rootDS->writeVector3( "boundingBox/min", bb.minBounds() + offsetToOrigin );
	rootDS->writeVector3( "boundingBox/max", bb.maxBounds() + offsetToOrigin );

	rootDS->writeVector3( "snaps/position", positionSnaps );
	rootDS->writeFloat( "snaps/angle", angleSnaps );

	BWResource::instance().purge( binFileName, true );

	binDS->setParent( BWResource::openSection(
		BWResource::getFilePath( binFileName ) ) );

	rootDS->save();
	if ( binDS->countChildren() > 0 )
		binDS->save();
	else
		::DeleteFile( bw_utf8tow( binFileName ).c_str() );

	binDS->setParent( NULL );

	return true;
}

/**
 * Return s, ignoring all characters not in validCharacters
 *
 * All characters are lowercased, thus validCharacters must be all lowercase
 */
static BW::string filterString( const BW::string& s, const BW::string& validCharacters )
{
	BW_GUARD;

	BW::string r;
	for (int i = 0; i < (int) s.length(); i++)
	{
		char testChar = s[i];
		testChar = tolower(testChar);
		if (validCharacters.find(testChar) != -1)
		{
			r += testChar;
		}
	}

	return r;
}

/*~ function WorldEditor.saveChunkPrefab
 *	@components{ worldeditor }
 *
 *	This function saves a chunk prefab to a file.
 *
 *	@param revealer The ChunkItemRevealer object of the chunk items to save
 *					as the chunk prefab.
 *	@param filename	The name of the file to save the chunk prefab to.
 *
 *	@return Returns a message if the save was successful, returns NULL otherwise.
 */
PyObject* ChunkPlacer::py_saveChunkPrefab( PyObject * args )
{
	BW_GUARD;

	// get args
	PyObject * pPyRev;
	char * name;
	if (!PyArg_ParseTuple( args, "Os", &pPyRev, &name ) ||
		!ChunkItemRevealer::Check( pPyRev ))
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.saveChunkPrefab() "
			"expects a ChunkItemRevealer and a filename" );
		return NULL;
	}

	ChunkItemRevealer* pRevealer = static_cast<ChunkItemRevealer*>( pPyRev );
	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );

	// Using this rather than the below section as the user is being prompted
	// with a save dialog these days rather than a simple text entry
	BW::string prefabName = BWResource::dissolveFilename( name );
	if (BWResource::getExtension( name ).empty())
		prefabName += ".prefab";

	// check for valid characters and add extension
	/*
	BW::string prefabFilename = filterString( name, "abcdefghijklmnopqrstuvwxyz1234567890_-" );

	if ( (prefabFilename.length() <= 7) ||
		(prefabFilename.substr(prefabFilename.length() - 7) != ".prefab") )
		prefabFilename += ".prefab";

	// find a directory and name for the prefab
	BW::string prefabName = "";
	{
		ChunkItemPtr pItem = *items.begin();
		Chunk* pChunk = pItem->chunk();
		DataSectionPtr pChunkSection = EditorChunkCache::instance( *pChunk ).pChunkSection();
		BW::string modelName = pItem->pOwnSect()->readString( "resource" );
		MF_ASSERT( !modelName.empty() );

		BW::string modelDir = BWResource::getFilePath( modelName );

		// strip back to the shell folder and change to prefab
		size_t shellIndex = modelName.find( "shells" );
		size_t postShellIndex = shellIndex + 7;
		size_t extensionIndex = modelName.find( ".model" );
		if (shellIndex == BW::string::npos)
			return NULL;
		if (postShellIndex == BW::string::npos)
			return NULL;
		if (extensionIndex == BW::string::npos)
			return NULL;
		MF_ASSERT(shellIndex < postShellIndex);
		MF_ASSERT(postShellIndex < extensionIndex);

		prefabName = modelName.substr(0, shellIndex);
		prefabName += "prefabs/";

		if (items.size() == 1)
		{
			// change from shell to prefab dir
			prefabName += modelName.substr(postShellIndex, extensionIndex - postShellIndex);
			prefabName += "_";
		}
		else
		{
			prefabName += modelName.substr(postShellIndex, modelName.find( "/", postShellIndex ) + 1 - postShellIndex);
			prefabName += "groups/";
		}

		prefabName += prefabFilename;
	}
	*/

	if (!savePrefab( prefabName, items ))
		return NULL;


	INFO_MSG( "Saved chunk prefab %s\n", prefabName.c_str() );

	BW::string returnMessage = LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_PLACER_PREFAB/SAVED_PREFAB_MSG", prefabName );
	return Py_BuildValue( "s", returnMessage.c_str() );
}

class IdMapping
{
public:
	UniqueID find(UniqueID oldID, bool realObject)
	{
		BW_GUARD;

		UniqueID newID = UniqueID::zero();

		IdMap::iterator it = idMap_.find( oldID );
		if (it != idMap_.end())
		{
			newID = it->second;
		}
		else
		{
			// else make a new id and push onto the map
			newID = UniqueID::generate();
			idMap_.insert( std::pair<UniqueID, UniqueID>( oldID, newID ) );
		}

		MF_ASSERT(newID != UniqueID::zero());

		// update info about the new object
		ObjectMap::iterator i = newObjectMap_.find( newID );
		if (i != newObjectMap_.end())
		{
			if (realObject)
			{
				// can't have two objects of the same id
				MF_ASSERT(i->second.realObject == false);
				i->second.realObject = true;
			}
			else
			{
				i->second.referenced++;
			}
		}
		else
		{
			ObjectInfo inf;
			if (realObject)
				inf.realObject = true;
			else
				inf.referenced = 1;

			newObjectMap_.insert(std::pair<UniqueID, ObjectInfo>( newID, inf ));
		}

		return newID;
	}

	bool realObject(UniqueID newID)
	{
		BW_GUARD;

		ObjectMap::iterator i = newObjectMap_.find( newID );
		if (i == newObjectMap_.end())
			return false;

		return i->second.realObject;
	}

	int referenced(UniqueID newID)
	{
		BW_GUARD;

		ObjectMap::iterator i = newObjectMap_.find( newID );
		if (i == newObjectMap_.end())
			return 0;

		return i->second.referenced;
	}

private:
	typedef BW::map<UniqueID, UniqueID> IdMap;
	IdMap idMap_;

	class ObjectInfo
	{
	public:
		ObjectInfo() : realObject(false), referenced(0) {}
		bool realObject;	// is this an id for a real item
		int referenced;		// how many items reference this id
	};
	typedef BW::map<UniqueID, ObjectInfo> ObjectMap;
	ObjectMap newObjectMap_;
};

/**
 *	Creates new GUIDs based off the old ones, so new items won't conflict with
 *	something already in the scene.
 */
static void rewriteLinks( DataSectionPtr item, IdMapping& clusterMarkerIDmap,
						IdMapping& stationIDmap, IdMapping& stationGraphIDmap )
{
	BW_GUARD;

	if ( (item->sectionName() == "marker") || (item->sectionName() == "marker_cluster") )
	{
		BW::string myIdString = item->readString( "id", "" );
		MF_ASSERT(myIdString.length() > 0);
		item->writeString( "id", clusterMarkerIDmap.find( UniqueID(myIdString), true ).toString() );

		if ( item->openSection( "parentID", false ) )
		{
			BW::string parentIdString = item->readString( "parentID", "" );
			item->writeString( "parentID", clusterMarkerIDmap.find( UniqueID(parentIdString), false ).toString() );
		}
	}

	if (item->sectionName() == "station")
	{
		BW::string myIdString = item->readString( "id", "" );
		MF_ASSERT(myIdString.length() > 0);
		item->writeString( "id", stationIDmap.find( UniqueID(myIdString), true ).toString() );

		BW::string graphIdString = item->readString( "graph", "" );
		MF_ASSERT(graphIdString.length() > 0);
		item->writeString( "graph", stationGraphIDmap.find( UniqueID(graphIdString), false ).toString() );

		DataSectionIterator iChildLinks;
		for(iChildLinks = item->begin(); iChildLinks != item->end(); iChildLinks++)
		{
			DataSectionPtr pLinkDS = *iChildLinks;
			if (pLinkDS->sectionName() != "link")
				continue;

			BW::string toIdString = pLinkDS->readString( "to", "" );
			MF_ASSERT(toIdString.length() > 0);
			pLinkDS->writeString( "to", stationIDmap.find( UniqueID(toIdString), false ).toString() );
		}
	}
}

/**
 *	Check that all the links are valid, in case the user didn't copy
 *	everything in the graph to the prefab etc
 */
static void verifyLinks( DataSectionPtr item,
						 BW::vector<DataSectionPtr>& sectionsToErase,
						 IdMapping& clusterMarkerIDmap,
						 IdMapping& stationIDmap, IdMapping& stationGraphIDmap )
{
	BW_GUARD;

	if ( item->sectionName() == "marker_cluster" )
	{
		// check that the reference count is correct, remove if not referenced
		BW::string myIdString = item->readString( "id", "" );
		MF_ASSERT(myIdString.length() > 0);
		int references = clusterMarkerIDmap.referenced( UniqueID(myIdString) );
		if (references == 0)
		{
			sectionsToErase.push_back(item);
			return;
		}
		else
		{
			item->writeInt( "number_children", references );
		}
	}

	if ( (item->sectionName() == "marker") || (item->sectionName() == "marker_cluster") )
	{
		// remove links to parents that don't exist
		if ( item->openSection( "parentID", false ) )
		{
			BW::string parentIdString = item->readString( "parentID", "" );
			if ( !clusterMarkerIDmap.realObject( UniqueID(parentIdString) ) )
			{
				item->writeString( "parentID", UniqueID::zero().toString() ); 
			}
		}
	}

	if ( item->sectionName() == "station" )
	{
		// remove links to stations that don't exist
		BW::vector<DataSectionPtr> linksToErase;
		DataSectionIterator iChildLinks;
		for(iChildLinks = item->begin(); iChildLinks != item->end(); iChildLinks++)
		{
			DataSectionPtr pLinkDS = *iChildLinks;
			if (pLinkDS->sectionName() != "link")
				continue;

			BW::string toIdString = pLinkDS->readString( "to", "" );
			MF_ASSERT(toIdString.length() > 0);
			if ( !stationIDmap.realObject( UniqueID(toIdString) ) )
			{
				linksToErase.push_back(pLinkDS);
				continue;
			}
		}

		for (BW::vector<DataSectionPtr>::iterator li = linksToErase.begin();
			li != linksToErase.end(); li++)
		{
			item->delChild(*li);
		}
	}
}


/** Find an offset to move bb by to fit it in container */
static Vector3 findBoundingBoxOffset( const BoundingBox& bb, const BoundingBox& container )
{
	BW_GUARD;

	Vector3 offset = Vector3::zero();

	Vector3 minDiff = bb.minBounds() - container.minBounds();
	Vector3 maxDiff = bb.maxBounds() - container.maxBounds();

	if (minDiff.x < 0.f)
		offset.x = -minDiff.x;
	else if (maxDiff.x > 0.f)
		offset.x = -maxDiff.x;

	if (minDiff.y < 0.f)
		offset.y = -minDiff.y;
	else if (maxDiff.y > 0.f)
		offset.y = -maxDiff.y;

	if (minDiff.z < 0.f)
		offset.z = -minDiff.z;
	else if (maxDiff.z > 0.f)
		offset.z = -maxDiff.z;

	return offset;
}

/**
 * Snap offset to snapAmount, but only by increasing the magnitude of offset
 */
static float snapOffset( float offset, float snapAmount )
{
	BW_GUARD;

	if (snapAmount > 0.f)
	{
		float v = offset;
		Snap::value( v, snapAmount );

		if (offset > 0.f)
		{
			if (v < offset)
				v += snapAmount;
		}
		else if (offset < 0)
		{
			if (v > offset)
				v -= snapAmount;
		}

		return v;

	}
	return offset;
}

// Used by items (ie, EditorChunkTerrain) to load their binary data from.
DataSectionPtr prefabBinarySection;

/*~ function WorldEditor.loadChunkPrefab
 *	@components{ worldeditor }
 *
 *	This function loads a chunk prefab from the specified filename and
 *	places the chunk items at the given ToolLocator's position.
 *
 *	@param filename The name of the chunk prefab file to load.
 *	@param locator	The ToolLocator or MatrixProvider which 
 *					will provide the position in the space 
 *					to load the prefab to.
 *
 *	@return Returns a ChunkItemGroup object of the loaded chunk items from the 
 *			loaded chunk prefab.
 */
PyObject* ChunkPlacer::py_loadChunkPrefab( PyObject * args )
{
	BW_GUARD;

	// get the arguments
	char * prefabName = 0;
	PyObject * pPyLoc = NULL;
	if (!PyArg_ParseTuple( args, "sO", &prefabName, &pPyLoc) ||
		(!ToolLocator::Check(pPyLoc) && !MatrixProvider::Check( pPyLoc )))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.loadChunkPrefab() "
			"expects a string and a ToolLocator or MatrixProvider" );
		return NULL;
	}

	// get the data section for it
	DataSectionPtr prefabSect = BWResource::openSection( prefabName );
	if (!prefabSect)
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.loadChunkPrefab() "
			"couldn't open prefab" );
		return NULL;
	}

	BoundingBox prefabBB = BoundingBox(
		prefabSect->readVector3( "boundingBox/min", Vector3( FLT_MAX, FLT_MAX, FLT_MAX ) ),
		prefabSect->readVector3( "boundingBox/max", Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX ) ) );

	Vector3 positionSnaps = prefabSect->readVector3( "snaps/position" );

	DataSectionPtr prefabChunkSect = prefabSect->openSection( "chunks" );
	DataSectionPtr prefabItemSect = prefabSect->openSection( "items" );

	// If there's no chunk tag, likely it's an old format prefab file with only
	// chunks, use the root section in this case
	if (!prefabChunkSect)
		prefabChunkSect = prefabSect;

	if (prefabItemSect->findChild("terrain"))
	{
		prefabBinarySection = BWResource::openSection(
			BWResource::changeExtension( prefabName, ".pdata" ) );
	}

	// find out where it goes from the locator
	Vector3 locatorPosition(0, 0, 0);

	if (ToolLocator::Check( pPyLoc ))
	{
		ToolLocator * pLoc = static_cast<ToolLocator*>( pPyLoc );
		locatorPosition = pLoc->transform().applyToOrigin();
	}
	else if ( MatrixProvider::Check( pPyLoc ) )
	{
		Matrix mat;
		static_cast<MatrixProvider*>(pPyLoc)->matrix( mat );
		locatorPosition = mat.applyToOrigin();
	}
	

	DataSectionPtr terrainTranslation = prefabItemSect->openSection( "terrain/prefabTransform/row3" );
	if (terrainTranslation)
	{
		// Using the grid lines nearest to the prefab's centre, to match the "save".
		locatorPosition -= prefabBB.centre();
		const float gridSize = ChunkManager::instance().cameraSpace()->gridSize();
		locatorPosition.x = Snap::value( locatorPosition.x, gridSize );
		locatorPosition.z = Snap::value( locatorPosition.z, gridSize );

	}

	Snap::vector3( locatorPosition, positionSnaps );

	// check if bounding box out of range

	BoundingBox spaceBB( ChunkManager::instance().cameraSpace()->gridBounds() );

	if (!(prefabBB == BoundingBox::s_insideOut_))
	{
		BoundingBox bb = BoundingBox( prefabBB.minBounds() + locatorPosition, prefabBB.maxBounds() + locatorPosition );



		if ( !(spaceBB.intersects( bb.minBounds() ) &&
			spaceBB.intersects( bb.maxBounds() )) )
		{
			// User hasn't placed the bb in the world, find the offset
			Vector3 offset = findBoundingBoxOffset( bb, spaceBB );

			offset.x = snapOffset( offset.x, positionSnaps.x );
			offset.y = snapOffset( offset.y, positionSnaps.y );
			offset.z = snapOffset( offset.z, positionSnaps.z );

			locatorPosition += offset;
		}
	}

	// TODO: we should check the whole bounding box is writable
	if (!WorldManager::instance().isBoundingBoxInWriteableChunk( prefabBB, locatorPosition ))
	{
		ERROR_MSG( "There's non-writable chunk inside the prefab's bounding box.\n" );
		Py_RETURN_NONE;
	}

	if (terrainTranslation)
	{
		locatorPosition.y = 0.f;
	}


	// select the items that are placed
	ChunkItemRevealer::ChunkItems newItems;

	bool success = true;

	// create new data sections
	BW::list<DataSectionPtr> newChunkSections;
	BW::list<BW::string> newChunkSectionNames;
	DataSectionIterator it;
	for (it = prefabChunkSect->begin(); it != prefabChunkSect->end(); ++it)
	{
		BW::string newChunkName;
		Chunk * pNearbyChunk = 
			ChunkManager::instance().cameraSpace()->column( locatorPosition )->pOutsideChunk();

		if (!pNearbyChunk)
		{
			ERROR_MSG( "WorldEditor.loadChunkPrefab() - Prefab too large" );
			Py_RETURN_NONE;
		}

		DataSectionPtr pDS = utilCreateInsideChunkDataSection(
			pNearbyChunk, newChunkName );

		if (!pDS)
		{
			Py_RETURN_NONE;
		}

		utilCloneChunkDataSection( *it, pDS, newChunkName );

		newChunkSections.push_back( pDS );
		newChunkSectionNames.push_back( newChunkName );
	}

	BW::list<DataSectionPtr> newItemSections;
	if (prefabItemSect)
	{
		for (it = prefabItemSect->begin(); it != prefabItemSect->end(); ++it)
		{
			DataSectionPtr pDS = new XMLSection( (*it)->sectionName() );
			if (!pDS)
				Py_RETURN_NONE;

			pDS->copy( *it );

			newItemSections.push_back( pDS );
		}
	}

	// toggle this to keep/remove links
	static const bool keepLinks = true;

	// Used to store the mappings between prefabed linker GUIDs and their
	// newly created copies
	EditorChunkItemLinkableManager::GuidToGuidMap	linkerGuidMapping;

	// preprocessing: convert to new values
	IdMapping clusterMarkerIDmap;
	IdMapping stationIDmap;
	IdMapping stationGraphIDmap;
	BW::list<DataSectionPtr>::iterator i;
	for (i = newChunkSections.begin(); i != newChunkSections.end(); ++i)
	{
		DataSectionPtr pDS = *i;

		// move it to the locator's position
		Matrix chunkTransform = pDS->readMatrix34( "transform" );
		chunkTransform.postTranslateBy( locatorPosition );
		pDS->writeMatrix34( "transform", chunkTransform );

		// move the bb to the new location
		Vector3 bbMin = pDS->readVector3( "boundingBox/min" );
		bbMin += locatorPosition;
		pDS->writeVector3( "boundingBox/min", bbMin );

		Vector3 bbMax = pDS->readVector3( "boundingBox/max" );
		bbMax += locatorPosition;
		pDS->writeVector3( "boundingBox/max", bbMax );

		// look inside the chunk prefab for linker objects.
		DataSectionIterator iChild;
		for(iChild = pDS->begin(); iChild != pDS->end(); ++iChild)
		{
			DataSectionPtr pChildDS = *iChild;

			WorldManager::instance().linkerManager().updateLinkerGuid( pChildDS, linkerGuidMapping );

			if (keepLinks)
				rewriteLinks( pChildDS, clusterMarkerIDmap, stationIDmap, stationGraphIDmap );
		}
	}

	// Preprocess the items
	for (i = newItemSections.begin(); i != newItemSections.end(); ++i)
	{
		WorldManager::instance().linkerManager().updateLinkerGuid( *i, linkerGuidMapping );

		if (keepLinks)
		{
			rewriteLinks( *i, clusterMarkerIDmap, stationIDmap, stationGraphIDmap );
		}
	}

	if (keepLinks)
	{
		// preprocessing: check links between clusters and markers
		for (i = newChunkSections.begin(); i != newChunkSections.end(); ++i)
		{
			DataSectionPtr pDS = *i;

			// look inside the chunk prefab
			BW::vector<DataSectionPtr> sectionsToErase;
			DataSectionIterator iChild;
			for(iChild = pDS->begin(); iChild != pDS->end(); iChild++)
			{
				DataSectionPtr pChildDS = *iChild;

				verifyLinks( pChildDS, sectionsToErase, clusterMarkerIDmap,
					stationIDmap, stationGraphIDmap );
			}

			// erase things not needed
			for (BW::vector<DataSectionPtr>::iterator ei = sectionsToErase.begin();
				ei != sectionsToErase.end(); ei++)
			{
				pDS->delChild(*ei);
			}
		}

		BW::vector<DataSectionPtr> sectionsToErase;
		for (i = newItemSections.begin(); i != newItemSections.end(); i++)
		{
			verifyLinks( *i, sectionsToErase, clusterMarkerIDmap,
				stationIDmap, stationGraphIDmap );
		}
		for (BW::vector<DataSectionPtr>::iterator ei = sectionsToErase.begin();
			ei != sectionsToErase.end(); ei++)
		{
			newItemSections.erase( std::find( newItemSections.begin(),
				newItemSections.end(), *ei ) );
		}

	}

	// load all the chunks in the section, with new chunk names
	BW::list<BW::string>::iterator ni;
	for (i = newChunkSections.begin(), ni = newChunkSectionNames.begin();
		 i != newChunkSections.end();
		 i++, ni++)
	{
		DataSectionPtr pDS = *i;
		BW::string& chunkName = *ni;
		Chunk* pChunk = utilCreateChunk( pDS, chunkName );
		if (!pChunk)
			return NULL;


		// if not in the space -> can't place this chunk
		if ( !(spaceBB.intersects( pChunk->boundingBox().minBounds() ) &&
				spaceBB.intersects( pChunk->boundingBox().maxBounds() )) )
		{
			PyErr_SetString( PyExc_ValueError, "WorldEditor.loadChunkPrefab() "
				"The newly created chunk is not inside the editable area." );
			success = false;
			break;
		}

		ChunkSpace::Column* col = ChunkManager::instance().cameraSpace()->column( pChunk->centre(), false );
		if (!col)
		{
			PyErr_Format( PyExc_ValueError,
				"Couldn't find column at %f, %f, %f", 
				pChunk->centre().x, pChunk->centre().y, pChunk->centre().z );
			success = false;
			break;
		}

		if (!col->pOutsideChunk())
		{
			PyErr_Format( PyExc_ValueError,
				"Couldn't find outside chunk at %f, %f, %f", 
				pChunk->centre().x,	pChunk->centre().y, pChunk->centre().z );
			success = false;
			break;
		}

		// and tell the cache that it has arrived!
		EditorChunkCache::instance( *pChunk ).edArrive( true );

		// now add an undo which deletes it
		UndoRedo::instance().add(
			new ChunkExistenceOperation( pChunk, false ) );

		// tell the chunk we just cloned it (true = don't give new IDs to markers / stations)
		EditorChunkCache::instance( *pChunk ).edPostClone( keepLinks );

		// select the appropriate shell model
		ChunkItemPtr newItem = EditorChunkCache::instance(*pChunk).getShellModel();

		if (newItem)
		{
			newItems.push_back( newItem );
		}

		// This is a bit of a hack to enable moving multiple chunks at once
		// Otherwise, the HullTree doesn't get updated, so the portals
		// can't find the chunk they're suppossed to connect to
		// ie, without this moving > 1 chunk at a time will result in
		// unresolved portals
		ChunkManager::instance().camera( Moo::rc().invView(), ChunkManager::instance().cameraSpace() );
	}

	BW::vector< EditorChunkTerrain * > modifiedTerrains;
	if (success)
	{
		// Place the items
		for (i = newItemSections.begin(); i != newItemSections.end(); i++)
		{
			DataSectionPtr itemPrefabSection = *i;
			bool isTerrain = itemPrefabSection->sectionName() == "terrain";
			bool isVLO = itemPrefabSection->sectionName() == "vlo";
			Matrix itemTransform = itemPrefabSection->readMatrix34( "prefabTransform" );
			itemTransform.postTranslateBy( locatorPosition );

			Chunk* pChunk = NULL;
			if ( !isTerrain )
			{
				pChunk = ChunkManager::instance().cameraSpace()->findChunkFromPointExact(
					itemTransform.applyToOrigin() );
			}
			else
			{
				pChunk = EditorChunk::findOutsideChunk( itemTransform.applyToOrigin() );
			}

			if (!pChunk || !pChunk->loaded())
			{
				PyErr_SetString( PyExc_ValueError, "WorldEditor.loadChunkPrefab() "
					"couldn't find chunk to place item." );
				success = false;
				break;
			}

			Matrix localTransform;
			localTransform.multiply( itemTransform, pChunk->transformInverse() );

			DataSectionPtr chunkSection = EditorChunkCache::instance( *pChunk ).pChunkSection();

			//if it is a terrain section, then remove the old one,
			//we don't want more than one copy in the chunk file.
			if ( isTerrain )
			{
				// delete the old chunk terrain entry from xml
				chunkSection->delChild( "terrain" );
				// delete the old chunk terrain item
				EditorChunkTerrain* pEct = static_cast<EditorChunkTerrain*>(
					ChunkTerrainCache::instance( *pChunk ).pTerrain());
				if (pEct)
				{
					pChunk->delStaticItem( pEct );
					// set up an undo which creates it
					UndoRedo::instance().add(
						new ChunkItemExistenceOperation( pEct, pChunk ) );
				}	
			}

			DataSectionPtr newSection = chunkSection->newSection(
					itemPrefabSection->sectionName() );
			newSection->copy( itemPrefabSection );
			newSection->delChild( "prefabTransform" );

			if (newSection->openSection( "transform" ))
				newSection->writeMatrix34( "transform", localTransform );
			if (newSection->openSection( "position" ))
				newSection->writeVector3( "position", localTransform.applyToOrigin() );

			// create the new item
			ChunkItemFactory::Result result = pChunk->loadItem( newSection );
			if ( result && result.onePerChunk() ) 
 			{ 
				// output a warning, and create an undo operation.
 				WARNING_MSG( result.errorString().c_str() ); 
 				UndoRedo::instance().add( 
 					new ChunkItemExistenceOperation( result.item(), pChunk ) ); 

				// delete the old light from the list of items to select
				ChunkItemRevealer::ChunkItems::iterator oldItem =
					std::find( newItems.begin(), newItems.end(), result.item() );
				if ( oldItem != newItems.end() )
					newItems.erase( oldItem );

				// replace it with the new one
 				result = pChunk->loadItem( newSection ); 
 			}
			if (!result)
			{
				PyErr_SetString( PyExc_ValueError, "WorldEditor.loadChunkPrefab() "
					"error creating item from given section" );
				success = false;
				break;
			}

			// get the new item out of the chunk
			ChunkItemPtr pItem = result.item();
			if (!pItem)
			{
				PyErr_SetString( PyExc_EnvironmentError, "WorldEditor.loadChunkPrefab() "
					"Couldn't create Chunk Item" );
				success = false;
				break;
			}

			BW::set<Chunk*> chunks;
			BoundingBox globalBB;
			pItem->edWorldBounds( globalBB );
			WorldManager::instance().collectOccupiedChunks( globalBB, pItem->chunk(), chunks );
			WorldManager::instance().changedChunks( chunks, *pItem );

			// If it was terrain the regenerate the lod texture
			if ( isTerrain )
			{
				EditorChunkTerrain* pEct = 
					static_cast<EditorChunkTerrain*>(ChunkTerrainCache::instance( *pChunk ).pTerrain());
				if (pEct)
				{
					modifiedTerrains.push_back( pEct );
					pEct->block().rebuildLodTexture(pEct->chunk()->transform());
				}
				
			}

			// ok, everyone's happy then. so add an undo which deletes it
			UndoRedo::instance().add(
				new ChunkItemExistenceOperation( pItem, NULL ) );

			if (!isVLO)
			{
				// Tell pitem we just cloned it from item
				if (keepLinks)
				{
					if ( (pItem->edDescription() != "marker") &&
						(pItem->edDescription() != "marker cluster") &&
						(pItem->edDescription() != "patrol node") )
					{
						pItem->edPostClone( NULL );
					}
				}
				else
				{
					pItem->edPostClone( NULL );
				}
			}
			pItem->edTransform( localTransform, false );

			newItems.push_back( pItem );
		}
	}

	// if no good, undo what was just done
	if ( !success )
	{
		UndoRedo::instance().barrier( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_PLACER_PREFAB/LOAD_PREFAB_BARRIER" ), false );
		UndoRedo::instance().undo();
		if (!PyErr_Occurred())
		{
			PyErr_SetString( PyExc_ValueError, 
				"Unknown error trying to place prefab." );
		}
		return NULL;
	}

	// If linker objects have been prefabed, inform the linker manager so
	// that it can update its representation of the scene
	if (linkerGuidMapping.size())
	{
		WorldManager::instance().linkerManager().updateMappedLinkers( linkerGuidMapping );
	}

	if (modifiedTerrains.size() != 0)
	{
		for (BW::vector< EditorChunkTerrain * >::iterator i = modifiedTerrains.begin(); i != modifiedTerrains.end(); ++i)
		{
			(*i)->onEditHeights();
			// also change left & bottom chunks' heightmap
			EditorChunkTerrain* pEct = (*i)->neighbour( EditorChunkTerrain::WEST );
			if (pEct)
			{
				pEct->onEditHeights();
			}

			pEct = (*i)->neighbour( EditorChunkTerrain::SOUTH );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::EAST );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::NORTH );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::NORTH_WEST );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::NORTH_EAST );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::SOUTH_WEST );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::SOUTH_EAST );
			if (pEct)
			{
				pEct->onEditHeights();
			}

		}

	}

	WorldManager::instance().setSelection( newItems );

	// set a barrier
	UndoRedo::instance().barrier( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_PLACER_PREFAB/LOAD_PREFAB_BARRIER" ), false );

	return new ChunkItemGroup( newItems );
}
BW_END_NAMESPACE

