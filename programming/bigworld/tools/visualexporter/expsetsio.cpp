#include "expsets.hpp"
#include "resmgr/dataresource.hpp"

BW_BEGIN_NAMESPACE

bool ExportSettings::readSettings( DataSectionPtr pSection )
{
	if( pSection )
	{
		exportMode_ = (ExportMode)pSection->readInt( "exportMode", exportMode_ );
		allowScale_ = pSection->readBool( "allowScale", allowScale_ );
		bumpMapped_ = pSection->readBool( "bumpMapped", bumpMapped_ );
		keepExistingMaterials_ = pSection->readBool( "keepExistingMaterials", keepExistingMaterials_ );
		snapVertices_ = pSection->readBool( "snapVertices", snapVertices_ );

		boneCount_ = pSection->readInt( "boneCount", boneCount_ );
		visualCheckerEnable_ = pSection->readBool( "visualCheckerEnable", visualCheckerEnable_ );
		fixCylindrical_ = pSection->readBool( "fixCylindrical", fixCylindrical_ );
		useLegacyOrientation_ = pSection->readBool( "useLegacyOrientation", useLegacyOrientation_ );

		return true;
	}
	return false;

}

bool ExportSettings::readSettings( const BW::string &fileName )
{
	DataResource file( fileName, RESOURCE_TYPE_XML );

	DataSectionPtr pRoot = file.getRootSection( );

	return readSettings( pRoot );
}

bool ExportSettings::writeSettings( DataSectionPtr pSection )
{
	if( pSection )
	{
		pSection->writeInt( "exportMode", exportMode_ );
		pSection->writeBool( "allowScale", allowScale_ );
		pSection->writeBool( "bumpMapped", bumpMapped_ );
		pSection->writeBool( "keepExistingMaterials", keepExistingMaterials_ );
		pSection->writeBool( "snapVertices", snapVertices_ );

		pSection->writeInt( "boneCount", boneCount_ );
		pSection->writeBool( "visualCheckerEnable", visualCheckerEnable_ );
		pSection->writeBool( "fixCylindrical", fixCylindrical_ );
		pSection->writeBool( "useLegacyOrientation", useLegacyOrientation_ );
		return true;
	}
	return false;
}

bool ExportSettings::writeSettings( const BW::string &fileName )
{
	DataResource file( fileName, RESOURCE_TYPE_XML );

	DataSectionPtr pRoot = file.getRootSection( );

	if (writeSettings( pRoot ))
	{
		file.save();

		return true;
	}
	return false;
}

BW_END_NAMESPACE

// expsetsio.cpp
