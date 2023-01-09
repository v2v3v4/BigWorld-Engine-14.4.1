#include "pch.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/zip_section.hpp"
#include "resmgr/xml_section.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This tests saving an XML DataResource.
 */
TEST_F( ResMgrUnitTestHarness, XMLDataResource_Save )
{
	CHECK( this->isOK() );

	DataResource dataRes1( "xmldataresource.xml", RESOURCE_TYPE_XML );
	CHECK( dataRes1.save() == DataHandle::DHE_NoError );

	DataResource dataRes2( "", RESOURCE_TYPE_XML );
	CHECK( dataRes2.save() == DataHandle::DHE_SaveFailed );
}

BW_END_NAMESPACE

// test_data_resource.cpp

