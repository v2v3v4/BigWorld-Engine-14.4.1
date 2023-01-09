#include "expsets.hpp"
#include "resmgr/dataresource.hpp"

BW_BEGIN_NAMESPACE

bool ExportSettings::readSettings( DataSectionPtr pSection )
{
	if( pSection )
	{		
		allowScale_ = pSection->readBool( "allowScale", allowScale_ );
		exportNodeAnimation_ = pSection->readBool( "exportNodeAnimation", exportNodeAnimation_ );
		referenceNodesFile_ = pSection->readString( "referenceNodesFile", referenceNodesFile_ );
		exportCueTrack_ = pSection->readBool( "exportCueTrack", exportCueTrack_ );	
		useLegacyOrientation_ = pSection->readBool( "useLegacyOrientation", useLegacyOrientation_ );

		return true;
	}
	return false;
}

bool ExportSettings::writeSettings( DataSectionPtr pSection )
{
	if( pSection )
	{		
		pSection->writeBool( "allowScale", allowScale_ );
		pSection->writeBool( "exportNodeAnimation", exportNodeAnimation_ );
		pSection->writeString( "referenceNodesFile", referenceNodesFile_ );
		pSection->writeBool( "exportCueTrack", exportCueTrack_ );
		pSection->writeBool( "useLegacyOrientation", useLegacyOrientation_ );

		return true;
	}
	return false;
}


bool ExportSettings::readSettings( const char* filename )
{
	DataResource file( filename, RESOURCE_TYPE_XML );

	DataSectionPtr pRoot = file.getRootSection( );

	return readSettings( pRoot );
}

bool ExportSettings::writeSettings( BW::string fileName )
{
	DataResource file( fileName, RESOURCE_TYPE_XML );

	DataSectionPtr pRoot = file.getRootSection( );

	if (writeSettings(pRoot))
	{
		file.save();
		return true;
	}
	return false;

}

BW_END_NAMESPACE

// expsetsio.cpp
