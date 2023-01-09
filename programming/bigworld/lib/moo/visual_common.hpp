#ifndef VISUAL_COMMON_HPP
#define VISUAL_COMMON_HPP

#include "physics2/bsp.hpp"
#include "physics2/worldtri.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/primitive_file.hpp"

#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

typedef BW::vector< BW::string > 	BSPMaterialIDs;

/**
 * 	This class' reason for existence is to hold the common code used to
 * 	load a visual file.
 *
 * 	The VISUAL template parameter is supposed to be a class very similar to
 * 	Moo::Visual. See individual functions for requirements for VISUAL class.
 */
template <class VISUAL>
class VisualLoader
{
	BW::string		resourceBaseName_;
	BW::string		visualResID_;
	BW::string		primitivesResID_;
	DataSectionPtr	pVisualSection_;

public:
	static bool visualFileExists( const BW::StringRef& resourceName,
		bool useProcessedData = true );

	static DataSectionPtr loadVisual( const BW::StringRef& resourceName,
		bool useProcessedData = true );

	VisualLoader( const BW::string& resourceBaseName, 
		bool useProcessedData = true );

	DataSectionPtr& getRootDataSection() { return pVisualSection_; }

	const BW::string& getVisualResID() const { return visualResID_; }
	const BW::string& getPrimitivesResID() const { return primitivesResID_; }

	bool loadBSPTree( typename VISUAL::BSPTreePtr& pBSPTree,
			BSPMaterialIDs& materialIDs );
	bool loadBSPTreeNoCache( typename VISUAL::BSPTreePtr& pBSPTree,
			BSPMaterialIDs& materialIDs );
	void setBoundingBox( BoundingBox& boundingBox );

	void collectMaterials( typename VISUAL::Materials& materials );
	void remapBSPMaterialFlags( BSPTree& bspTree,
			const typename VISUAL::Materials& materials,
			const BSPMaterialIDs& materialIDs,
			BSPMaterialIDs& missingMaterialIDs );

private:
	static DataSectionPtr loadVisual( BW::string& resourceBaseName,
		BW::string& actualResourceName, bool useProcessedData = true );
};

template <class VISUAL>
bool VisualLoader<VISUAL>::visualFileExists(
	const BW::StringRef& resourceName, bool useProcessedData )
{
	BW_GUARD;
	return loadVisual( resourceName, useProcessedData ) != NULL;
}

template <class VISUAL>
DataSectionPtr VisualLoader<VISUAL>::loadVisual(
	const BW::StringRef& resourceName,
	bool useProcessedData )
{
	BW_GUARD;
	BW::string basename = resourceName.to_string();
	basename = basename.substr( 0, basename.find_last_of( '.' ) );
	BW::string actualName;

	return loadVisual( basename, actualName, useProcessedData );
}

template <class VISUAL>
DataSectionPtr VisualLoader<VISUAL>::loadVisual(
	BW::string& resourceBaseName,
	BW::string& actualResourceName, bool useProcessedData )
{
	BW_GUARD;
	DataSectionPtr dataSection = NULL;
	std::replace( resourceBaseName.begin(), resourceBaseName.end(), '\\', '/' );

	// Try .visual.processed file
	if (useProcessedData)
	{
		actualResourceName = resourceBaseName + ".visual.processed";
		dataSection = BWResource::openSection( actualResourceName );
	}

	if (!dataSection)
	{
		// Try a standard visual file
		actualResourceName = resourceBaseName + ".visual";
		dataSection = BWResource::openSection( actualResourceName );

		if (!dataSection)
		{
			// Try .static.visual file
			actualResourceName = resourceBaseName + ".static.visual";
			dataSection = BWResource::openSection( actualResourceName );
		}
	}

	return dataSection;
}


/**
 *	Constructor.
 */
template <class VISUAL>
VisualLoader< VISUAL >::VisualLoader( const BW::string& resourceBaseName,
	bool useProcessedData ) :
	resourceBaseName_( resourceBaseName ),
	visualResID_(), primitivesResID_(), pVisualSection_()
{
	BW_GUARD;
	pVisualSection_ =
		loadVisual( resourceBaseName_, visualResID_, useProcessedData );
	
	if (!pVisualSection_)
	{
		ASSET_MSG( "VisualLoader::VisualLoader: Failed to load visual file "
			"for %s\n", resourceBaseName.c_str() );
	}

	if (pVisualSection_)
	{
		primitivesResID_ = 
			pVisualSection_->readString( "primitivesName", resourceBaseName_ );
		primitivesResID_.append( ".primitives" );
	}
	else
	{
		primitivesResID_ = resourceBaseName_ + ".primitives";
	}
}

/**
 *	This method returns the BSP tree for the visual. It gets the BSP using
 * 	the following methods:
 * 		1. Look in BSP cache.
 * 		2. Load pre-generated BSP from file (see loadBSPTreeNoCache() method)
 *	The returned BSP is inserted into the BSP cache if it wasn't there before.
 *
 * 	The BSPTree is returned via the bspTreeProxy output parameter.
 *
 * 	Ruturns a boolean indicating whether the BSP tree uses primitive groups
 * 	for its collision and material flags (i.e. it's a bsp2 BSPTree).
 * 	A bsp2 BSPTree will need the primitive groups information to complete its
 * 	loading. This is done by calling the remapFlags() method of the BSPTree.
 * 	Non-bsp2 BSPTrees are fully loaded by this function. A NULL BSPTree is
 * 	returned if it is not found in the cache or on disk i.e. needs to be
 * 	generated.
 *
 * 	Requires:
 * 		- VISUAL::BSPCache& VISUAL::BSPCache::instance()
 * 		- VISUAL::BSPCache::find( BW::string ) returns VISUAL::BSPTreePtr
 * 		- VISUAL::BSPCache::add( BW::string, VISUAL::BSPTreePtr )
 * 		- VISUAL::loadBSPVisual( BW::string ) returns VISUAL::BSPTreePtr
 */
template <class VISUAL>
bool VisualLoader< VISUAL >::loadBSPTree( typename VISUAL::BSPTreePtr& pBSPTree,
		BSPMaterialIDs& materialIDs )
{
	BW_GUARD;
	bool shouldRemapFlags = false;
	pBSPTree = VISUAL::BSPCache::instance().find( visualResID_ );
	if (!pBSPTree)
	{
		shouldRemapFlags = this->loadBSPTreeNoCache( pBSPTree, materialIDs );
		if (pBSPTree)
		{
			VISUAL::BSPCache::instance().add( visualResID_, pBSPTree );
		}
	}

	return shouldRemapFlags;
}

/**
 *	This function finds and loads a bsp for a visual.
 *	At the moment (1.8), it goes like this :
 *	- <basename>_bsp.visual file
 *	- "bsp2" subfile in <basename>.primitives
 *	- "bsp" subfile in <basename>.primitives
 *	- "<basename>.bsp" file
 * 	Returns the BSPTree if found and whether it was loaded from the "bsp2"
 * 	sub-file.
 */
template <class VISUAL>
bool VisualLoader< VISUAL >::loadBSPTreeNoCache(
		typename VISUAL::BSPTreePtr& pBSPTree, BSPMaterialIDs& materialIDs )
{
	BW_GUARD;
	bool 		shouldRemapFlags = false;

	// Try loading _bsp.visual
	BW::string bspVisualResID = resourceBaseName_ + "_bsp.visual";
	// Potential infinite recursion. VISUAL::loadBSPVisual() should test
	// whether file exists and exit early if it doesn't.
	pBSPTree = VISUAL::loadBSPVisual( bspVisualResID );
	if (pBSPTree)
	{
		WARNING_MSG( "Separate BSP files (_bsp.visual) are deprecated "
				"in BigWorld 1.8. Please re-export %s\n",
				resourceBaseName_.c_str() );
	}
	else
	{
		DataSectionPtr pBSPSection;

		// Try loading from .primitives file
		DataSectionPtr pPrimitivesSection =
				PrimitiveFile::get( primitivesResID_ );
		if (pPrimitivesSection)
		{
			pBSPSection = pPrimitivesSection->findChild( "bsp2" );
			if (pBSPSection)
			{
				//TODO : this is the only codepath that will be valid for
				//BigWorld 1.9 (we are deprecating everything else.)
				shouldRemapFlags = true;

				BinaryPtr pBin =
						pPrimitivesSection->readBinary( "bsp2_materials" );
				if (pBin)
				{
					//Make a copy of pBin, as createFromBinary modifies the
					//data (see fn comment in XMLSection::createFromBinary)
					BinaryPtr pCopy = new BinaryBlock(pBin->data(), pBin->len(),
						"VisualLoader::loadBSPTreeNoCache");
					XMLSectionPtr pMaterialIDs = 
						XMLSection::createFromBinary( "root", pCopy );

					if (pMaterialIDs)
					{
						pMaterialIDs->readStrings( "id", materialIDs );
#ifdef EDITOR_ENABLED
						if (materialIDs.empty())
						{
							WARNING_MSG( "%s was exported using the old bsp2 "
									"exporter, and must be re-exported manually\n",
									resourceBaseName_.c_str() );
						}
#endif
					}
				}
			}
			else
			{
				pBSPSection = pPrimitivesSection->findChild( "bsp" );
				if (pBSPSection)
				{
					WARNING_MSG( "Primitives file contains \"bsp\" section, "
						"which is deprecated in BigWorld 1.8 and replaced"
						" with \"bsp2\" section.  Please re-export %s\n",
						resourceBaseName_.c_str() );
				}
			}
		}

		if (!pBSPSection)
		{
			// Try loading from .bsp file
			BW::string bspResID = resourceBaseName_ + ".bsp";
			pBSPSection = BWResource::openSection( bspResID );
			if (pBSPSection)
			{
				if (BWResource::isFileOlder( bspResID, visualResID_ ))
				{
					WARNING_MSG( "%s is older than %s\n", bspResID.c_str(),
							visualResID_.c_str() );
				}

				WARNING_MSG( "Separate BSP files (.bsp) are deprecated in "
					"BigWorld 1.8\nPlease re-export %s\n",
					resourceBaseName_.c_str() );
			}
		}

		BinaryPtr pBSPData;
		if (pBSPSection)
			pBSPData = pBSPSection->asBinary();

		if (pBSPData)
		{
			pBSPTree = BSPTreeTool::loadBSP( pBSPData );
		}
		else
		{
			//Hopefully this is a skinned object and doesn't require a BSP.
//			WARNING_MSG( "No BSP data found. Please re-export %s\n",
//				resourceBaseName_.c_str() );
		}
	}

	return shouldRemapFlags;
}

/**
 *	This method remaps the flags in the visual's BSP, from bsp material
 *	index to use the visual's material's collision flags / material kinds.
 *
 *	@param	bspMaterialIDs	ordered list of material names, one for each
 *							index currently in the world triangles of the bsp.
 */
template <class VISUAL>
void VisualLoader< VISUAL >::remapBSPMaterialFlags( BSPTree& bspTree,
		const typename VISUAL::Materials& materials,
		const BSPMaterialIDs& materialIDs,
		BSPMaterialIDs& missingBSPMaterials)
{
	BW_GUARD;
	int objectMaterialKind = pVisualSection_->readInt( "materialKind" );

	//construct bspMap given the bsp's bspMaterialIDs and
	//the visual's materials.
	//
	//go through the BSP's materials list, and for each one
	//add an entry to the bspMap that says what materialKind
	//to give triangles that have the material.
	BSPFlagsMap bspMap;
	for ( BSPMaterialIDs::const_iterator iterMatID = materialIDs.begin();
			iterMatID != materialIDs.end(); ++iterMatID )
	{
		const typename VISUAL::Material * pMaterial =
			&*materials.find( *iterMatID );

		// If material is not found this means the custom BSP was not
		// assigned materials from the object, so we didn't find the material
		// identifier in the object.  So that means we use the object's material
		// kind.
#ifdef EDITOR_ENABLED
	if (pMaterial == NULL)
	{

		WARNING_MSG( "Model %s.model BSP is using unreferenced material \"%s\". Please assign the correct materials and re-export the model.\n", resourceBaseName_.c_str(), (*iterMatID).c_str() );
		missingBSPMaterials.push_back( *iterMatID );
	}
#endif //EDITOR
		WorldTriangle::Flags flags = (pMaterial) ? pMaterial->getFlags( objectMaterialKind ) :
									WorldTriangle::packFlags( 0, objectMaterialKind );
		bspMap.push_back( flags );
	}

	if (bspMap.empty())
	{
		bspMap.push_back( WorldTriangle::packFlags( 0, objectMaterialKind ) );
	}

	bspTree.remapFlags( bspMap );
}

/**
 *	This function loops visits all the materials in the primitive groups
 * 	in the visual and adds them to materials.
 *
 * 	NOTE: This method is only used by the server because it usually does not
 * 	need to process all the information in the visual file.
 *
 * 	Requires:
 * 		- VISUAL::Materials::add( VISUAL::Material* )
 * 		- VISUAL::Material::Material( DataSectionPtr )
 */
template <class VISUAL>
void VisualLoader< VISUAL >::collectMaterials(
		typename VISUAL::Materials& materials )
{
	BW_GUARD;
	// Loop through all renderSet/geometry/primitiveGroup sections
	for ( DataSection::SearchIterator iterRenderSet =
			pVisualSection_->beginSearch( "renderSet" );
			iterRenderSet != pVisualSection_->endOfSearch(); ++iterRenderSet )
	{
		DataSectionPtr pRenderSetSection = *iterRenderSet;
		for ( DataSection::SearchIterator iterGeometry =
				pRenderSetSection->beginSearch( "geometry" );
				iterGeometry !=  pRenderSetSection->endOfSearch();
				++iterGeometry )
		{
			DataSectionPtr pGeometrySection = *iterGeometry;
			for ( DataSection::SearchIterator iterPrimGrp =
					pGeometrySection->beginSearch( "primitiveGroup" );
					iterPrimGrp !=  pGeometrySection->endOfSearch();
					++iterPrimGrp )
			{
				DataSectionPtr pPrimitiveGrpSection = *iterPrimGrp;
				DataSectionPtr pMaterialSection =
						pPrimitiveGrpSection->openSection( "material" );

				typename VISUAL::Material* pMaterial =
						new typename VISUAL::Material( pMaterialSection );
				materials.add( pMaterialSection->readString( "identifier" ),
						pMaterial );
			}
		}
	}
}

/**
 *	Sets the bounding box from the visual file.
 */
template <class VISUAL>
void VisualLoader< VISUAL >::setBoundingBox( BoundingBox& boundingBox )
{
	BW_GUARD;
	boundingBox.setBounds( pVisualSection_->readVector3( "boundingBox/min" ),
			pVisualSection_->readVector3( "boundingBox/max" ) );
}

}	// namespace Moo

BW_END_NAMESPACE

#endif // VISUAL_COMMON_HPP
