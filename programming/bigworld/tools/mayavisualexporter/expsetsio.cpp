#include "pch.hpp"
#include "expsets.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

bool ExportSettings::readSettings()
{
	return readSettings( settingFilename_ );
}

bool ExportSettings::readSettings( const BW::string & fileName, bool root )
{
	if (!BWResource::fileExists( fileName ))
	{
		return false;
	}

	DataResource file( fileName, RESOURCE_TYPE_XML );

	DataSectionPtr pRoot = file.getRootSection();
	if (root)
	{
		return readSettings( pRoot );
	}

	if (!pRoot)
	{
		return false;
	}

	DataSectionPtr pMetaData = pRoot->findChild( "metaData" );
	if (!pMetaData)
	{
		return false;
	}

	DataSectionPtr pSettings = pMetaData->findChild( "settings" );
	return readSettings( pSettings );
}

bool ExportSettings::readSettings( const DataSectionPtr & pRoot )
{
	if (!pRoot)
	{
		return false;
	}

	exportAnim_ = pRoot->readBool( "exportAnimation", exportAnim_ );
	exportMode_ = (ExportMode)pRoot->readInt( "exportMode", exportMode_ );
	maxBones_ = pRoot->readInt( "boneCount", maxBones_ );
	transformToOrigin_ = pRoot->readBool( "transformToOrigin", transformToOrigin_ );
	allowScale_ = pRoot->readBool( "allowScale", allowScale_ );
	bumpMapped_ = pRoot->readBool( "bumpMapped", bumpMapped_ );
	keepExistingMaterials_ = pRoot->readBool( "keepExistingMaterials", keepExistingMaterials_ );
	snapVertices_ = pRoot->readBool( "snapVertices", snapVertices_ );
	stripRefPrefix_ = pRoot->readBool( "stripRefPrefix", stripRefPrefix_ );
	referenceNode_ = pRoot->readBool( "useReferenceNode", referenceNode_ );
	BW::string refNodesFile = pRoot->readString("referenceNodesFile", "");
	if (!refNodesFile.empty())
	{
		BWResource::resolveToAbsolutePath( refNodesFile );
		referenceNodesFileAbs_ = refNodesFile;
	}
	disableVisualChecker_ = pRoot->readBool( "disableVisualChecker", disableVisualChecker_ );
	useLegacyScaling_ = pRoot->readBool( "useLegacyScaling", useLegacyScaling_ );
	fixCylindrical_ = pRoot->readBool( "fixCylindrical", fixCylindrical_ );
	useLegacyOrientation_ = pRoot->readBool( "useLegacyOrientation", useLegacyOrientation_ );
	sceneRootAdded_ = pRoot->readBool( "sceneRootAdded", sceneRootAdded_ );

	includeMeshes_ = pRoot->readBool( "includeMeshes", includeMeshes_ );
	includeEnvelopesAndBones_ = pRoot->readBool( "includeEnvelopesAndBones", includeEnvelopesAndBones_ );
	includeNodes_ = pRoot->readBool( "includeNodes", includeNodes_ );
	includeMaterials_ = pRoot->readBool( "includeMaterials", includeMaterials_ );
	includeAnimations_ = pRoot->readBool( "includeAnimations", includeAnimations_ );
	useCharacterMode_ = pRoot->readBool( "useCharacterMode", useCharacterMode_ );
	animationName_ = pRoot->readString( "animationName", animationName_ );
	includePortals_ = pRoot->readBool( "includePortals", includePortals_ );
	worldSpaceOrigin_ = pRoot->readBool( "worldSpaceOrigin", worldSpaceOrigin_ );
	unitScale_ = pRoot->readFloat( "unitScale", unitScale_ );
	localHierarchy_ = pRoot->readBool( "localHierarchy", localHierarchy_ );
	nodeFilter_ = (NodeFilter)pRoot->readInt( "nodeFilter", nodeFilter_ );

	copyExternalTextures_ = pRoot->readBool( "copyExternalTextures", copyExternalTextures_ );
	copyTexturesTo_ = pRoot->readString( "copyTexturesTo", copyTexturesTo_ );

	return true;
}

bool ExportSettings::writeSettings()
{
	return writeSettings( settingFilename_ );
}

bool ExportSettings::writeSettings( const BW::string & fileName, bool root )
{
	DataResource file( fileName, RESOURCE_TYPE_XML );

	DataSectionPtr pRoot = file.getRootSection( );
	if (root)
	{
		if (!writeSettings( pRoot ))
		{
			return false;
		}
		file.save();
		return true;
	}

	DataSectionPtr pMetaData = pRoot->findChild( "metaData" );
	if (!pMetaData)
	{
		pMetaData = pRoot->newSection( "metaData" );
		if (!pMetaData)
		{
			return false;
		}
	}
	else
	{
		pMetaData->delChild( "settings" );
	}

	DataSectionPtr settings = pMetaData->newSection( "settings" );

	if (!writeSettings( settings ))
	{
		return false;
	}
	file.save();
	return true;
}

bool ExportSettings::writeSettings( DataSectionPtr & pRoot )
{
	if(!pRoot)
	{
		return false;
	}

	pRoot->writeBool( "exportAnimation", exportAnim_ );
	pRoot->writeInt( "exportMode", exportMode_ );
	pRoot->writeInt( "boneCount", maxBones_ );
	pRoot->writeBool( "transformToOrigin", transformToOrigin_ );
	pRoot->writeBool( "allowScale", allowScale_ );
	pRoot->writeBool( "bumpMapped", bumpMapped_ );
	pRoot->writeBool( "keepExistingMaterials", keepExistingMaterials_ );
	pRoot->writeBool( "snapVertices", snapVertices_ );
	pRoot->writeBool( "stripRefPrefix", stripRefPrefix_ );
	pRoot->writeBool( "useReferenceNode", referenceNode_ );
	pRoot->writeString( "referenceNodesFile",
		BWResource::dissolveFilename( referenceNodesFileAbs_ ) );
	pRoot->writeBool( "disableVisualChecker", disableVisualChecker_ );
	pRoot->writeBool( "useLegacyScaling", useLegacyScaling_ );
	pRoot->writeBool( "fixCylindrical", fixCylindrical_ );
	pRoot->writeBool( "useLegacyOrientation", useLegacyOrientation_ );
	pRoot->writeBool( "sceneRootAdded", sceneRootAdded_ );

	pRoot->writeBool( "includeMeshes", includeMeshes_ );
	pRoot->writeBool( "includeEnvelopesAndBones", includeEnvelopesAndBones_ );
	pRoot->writeBool( "includeNodes", includeNodes_ );
	pRoot->writeBool( "includeMaterials", includeMaterials_ );
	pRoot->writeBool( "includeAnimations", includeAnimations_ );
	pRoot->writeBool( "useCharacterMode", useCharacterMode_ );
	pRoot->writeString( "animationName", animationName_ );
	pRoot->writeBool( "includePortals", includePortals_ );
	pRoot->writeBool( "worldSpaceOrigin", worldSpaceOrigin_ );
	pRoot->writeFloat( "unitScale", unitScale_ );
	pRoot->writeBool( "localHierarchy", localHierarchy_ );
	pRoot->writeInt( "nodeFilter", nodeFilter_ );

	pRoot->writeBool( "copyExternalTextures", copyExternalTextures_ );
	pRoot->writeString( "copyTexturesTo", copyTexturesTo_ );

	return true;
}

BW_END_NAMESPACE

