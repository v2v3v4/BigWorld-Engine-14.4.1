#include "pch.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"


BW_BEGIN_NAMESPACE

DataSectionPtr createXmlFromString( const char * string )
{
	BW::stringstream sstream;
	sstream << string;

	return XMLSection::createFromStream( "", sstream ).get();
}

bool hasChildren( const char * string, int numChildren )
{
	DataSectionPtr section = createXmlFromString(string);
	return section.exists() && section->countChildren() == numChildren;
}

int recursiveCountChildren( DataSectionPtr section )
{
	if (!section.exists())
		return 0;
	int count  = section->countChildren();
	for (int i = 0; i < section->countChildren(); ++i)
	{
		count += recursiveCountChildren( section->openChild( i ) );
	}
	return count;
}

bool hasAllChildren( const char * string, int numChildren )
{
	DataSectionPtr section = createXmlFromString(string);
	return section.exists() && recursiveCountChildren( section ) == numChildren;
}

TEST_F( ResMgrUnitTestHarness, XMLSection_SyntaxCheck )
{
	CHECK( this->isOK() );

	// Good XML
	CHECK( createXmlFromString("<root />").exists() );
	CHECK( createXmlFromString("<root></root>").exists() );
	CHECK( hasChildren("<root><a/></root>", 1) );
	CHECK( hasChildren("<root><a></a></root>", 1) );
	CHECK( hasChildren("<root><a b=\"a\"></a></root>", 1) );
	CHECK( hasChildren("<root a=\"a\"><c/></root>", 2) );
	CHECK( hasChildren("<root a=\"a\"><a/></root>", 2) );
	CHECK( hasChildren("<root><a/><a></a></root>", 2) );
	CHECK( hasChildren("<root><a/><a/></root>", 2) );
	CHECK( hasChildren("<root><a></a><a></a></root>", 2) );
	CHECK( hasAllChildren("<root><a b=\"a\"></a></root>", 2) );
	CHECK( hasAllChildren("<root><a b=\"a\" c=\"a\"></a></root>", 3) );
	CHECK( hasAllChildren("<root><a b=\"a\"><c/></a></root>", 3) );

	// Bad XML
	CHECK( !createXmlFromString("Bad XML<root></root>").exists() );
	CHECK( !createXmlFromString("<root><</root>").exists() );
	CHECK( !createXmlFromString("</root>").exists() );
	CHECK( !createXmlFromString("<root>").exists() );
	CHECK( !createXmlFromString("<root></a></root>").exists() );
	CHECK( !createXmlFromString("<root><a></a><a><a/></root>").exists() );
}

BW_END_NAMESPACE

// test_data_resource.cpp
