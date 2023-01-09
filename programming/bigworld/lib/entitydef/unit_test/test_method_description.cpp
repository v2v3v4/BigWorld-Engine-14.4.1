#include "pch.hpp"

#include "test_harness.hpp"

#include "entitydef/entity_method_descriptions.hpp"

#include "resmgr/xml_section.hpp"

BW_BEGIN_NAMESPACE

namespace
{

/**
 *	Utility method to get a DataSectionPtr given some XML
 */
DataSectionPtr getDataSectionForXML( const BW::string & dataSectionStr )
{
	BW::stringstream dataStrStream;
	dataStrStream << dataSectionStr;
	XMLSectionPtr xml = XMLSection::createFromStream( "", dataStrStream );
	return xml;
}

} // end anonymous namespace


TEST_F( EntityDefUnitTestHarness, MethodDescription_empty_signature )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_equal_simple )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args>"
		"		<strParam>		STRING	</strParam>"
		"		<floatParam>	FLOAT32	</floatParam>"
		"		<intParam>		INT32	</intParam>"
		"	</Args>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args>"
		"		<strParam>		STRING	</strParam>"
		"		<floatParam>	FLOAT32	</floatParam>"
		"		<intParam>		INT32	</intParam>"
		"	</Args>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_equal_signature )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args>"
		"		<strParam>		STRING	</strParam>"
		"		<floatParam>	FLOAT32	</floatParam>"
		"		<intParam>		INT32	</intParam>"
		"		<blobParam>		BLOB 	</blobParam>"
		"		<vec2Param>		VECTOR2	</vec2Param>"
		"		<vec3Param>		VECTOR3	</vec3Param>"
		"		<vec4Param>		VECTOR4	</vec4Param>"
		"		<mbxParam>		MAILBOX	</mbxParam>"
		"		<arrParam>		ARRAY <of> UNICODE_STRING </of>	</arrParam>"
		"		<tupleParam>	ARRAY <of> UINT16 </of>	</tupleParam>"
		"		<stupleParam>	ARRAY <of> FLOAT64 </of> <size> 4 </size> </stupleParam>"
		"		<dictParam> 	FIXED_DICT"
		"			<Properties>"
		"				<prop1>	<Type> INT64 </Type> </prop1>"
		"				<prop2>	<Type> UINT64 </Type> </prop2>"
		"			</Properties>"
		"		</dictParam>"
		"	</Args>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args>"
		"		<strParam>		STRING	</strParam>"
		"		<floatParam>	FLOAT32	</floatParam>"
		"		<intParam>		INT32	</intParam>"
		"		<blobParam>		BLOB 	</blobParam>"
		"		<vec2Param>		VECTOR2	</vec2Param>"
		"		<vec3Param>		VECTOR3	</vec3Param>"
		"		<vec4Param>		VECTOR4	</vec4Param>"
		"		<mbxParam>		MAILBOX	</mbxParam>"
		"		<arrParam>		ARRAY <of> UNICODE_STRING </of>	</arrParam>"
		"		<tupleParam>	ARRAY <of> UINT16 </of>	</tupleParam>"
		"		<stupleParam>	ARRAY <of> FLOAT64 </of> <size> 4 </size> </stupleParam>"
		"		<dictParam> 	FIXED_DICT"
		"			<Properties>"
		"				<prop1>	<Type> INT64 </Type> </prop1>"
		"				<prop2>	<Type> UINT64 </Type> </prop2>"
		"			</Properties>"
		"		</dictParam>"
		"	</Args>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_not_equal_simple )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testNotEqualMethod>"
		"	<Exposed/>"
		"	<Args>"
		"		<Param>	FLOAT32	</Param>"
		"	</Args>"
		"</testNotEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testNotEqualMethod>"
		"	<Exposed/>"
		"	<Args>"
		"		<Param>	STRING	</Param>"
		"	</Args>"
		"</testNotEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(!methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_not_equal_dict )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testNotEqualMethod>"
		"	<Exposed/>"
		"	<Args>"
		"		<dictParam> FIXED_DICT"
		"			<Properties>"
		"				<prop1>	<Type> INT64 </Type> </prop1>"
		"				<prop2>	<Type> UINT64 </Type> </prop2>"
		"			</Properties>"
		"		</dictParam>"
		"	</Args>"
		"</testNotEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testNotEqualMethod>"
		"	<Exposed/>"
		"	<Args>"
		"		<dictParam> FIXED_DICT"
		"			<Properties>"
		"				<prop1>	<Type> INT32 </Type> </prop1>"
		"				<prop2>	<Type> UINT64 </Type> </prop2>"
		"			</Properties>"
		"		</dictParam>"
		"	</Args>"
		"</testNotEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(!methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_not_equal_exposure )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Exposed/>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(!methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_not_equal_component )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(!methods.init(pMethodData2, MethodDescription::CELL, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_duplicate_in_component )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(!methods.init(pMethodData2, MethodDescription::BASE, "iface", "component1"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_entity_component_conflict )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", ""));
	CHECK(!methods.init(pMethodData2, MethodDescription::BASE, "iface", "component1"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_component_entity_conflict )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args/>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(!methods.init(pMethodData2, MethodDescription::BASE, "iface", ""));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_equal_mailbox )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args>"
		"		<mbxParam>		MAILBOX	</mbxParam>"
		"	</Args>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testEqualMethod>"
		"	<Args>"
		"		<mbxParam>		MAILBOX	</mbxParam>"
		"	</Args>"
		"</testEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}


TEST_F( EntityDefUnitTestHarness, MethodDescription_not_equal_param_name )
{
	DataSectionPtr pMethodData1 = getDataSectionForXML(
		"<methods>"
		"<testNotEqualMethod>"
		"	<Exposed/>"
		"	<Args>"
		"		<Param1>	STRING	</Param1>"
		"	</Args>"
		"</testNotEqualMethod>"
		"</methods>");

	CHECK( pMethodData1 );

	DataSectionPtr pMethodData2 = getDataSectionForXML(
		"<methods>"
		"<testNotEqualMethod>"
		"	<Exposed/>"
		"	<Args>"
		"		<Param2>	STRING	</Param2>"
		"	</Args>"
		"</testNotEqualMethod>"
		"</methods>");

	CHECK( pMethodData2 );

	EntityMethodDescriptions methods;

	CHECK(methods.init(pMethodData1, MethodDescription::BASE, "iface", "component1"));
	CHECK(!methods.init(pMethodData2, MethodDescription::BASE, "iface", "component2"));
}

BW_END_NAMESPACE

// test_stream.cpp
