#include "pch.hpp"

#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"

#include "compiled_space/binary_format.hpp"
#include "compiled_space/binary_writers/binary_format_writer.hpp"

#include "compiled_space/string_table.hpp"
#include "compiled_space/binary_writers/string_table_writer.hpp"

#include "compiled_space/asset_list.hpp"
#include "compiled_space/binary_writers/asset_list_writer.hpp"

BW_BEGIN_NAMESPACE

using namespace CompiledSpace;

namespace {
	const char* TEST_FILENAME = "binary_container_format_unit_test.bin";

	FourCC TEST_MAGIC1 = FourCC( "BIGS" );
	FourCC TEST_MAGIC2 = FourCC( "ABCD" );
	FourCC TEST_MAGIC3 = FourCC( "XYZW" );
	FourCC TEST_MAGIC4 = FourCC( "IJKL" );
	FourCC TEST_MAGIC5 = FourCC( "STUV" );

	uint32 TEST_VERSION1 = 0xb14b14b1;
	uint32 TEST_VERSION2 = 0x12345678;
	uint32 TEST_VERSION3 = 0x87654321;
	uint32 TEST_VERSION4 = 0x12348765;
	uint32 TEST_VERSION5 = 0x43215678;

	// TEST_DATA1 is a large programmatically generated blob to push it across 
	// system granularity boundary (64k +)
	const size_t TEST_DATA1_LENGTH = 65536 + 1234;

	const char* TEST_DATA2 = "abcdefghijklmnopqrstuvwxyz";
	const char* TEST_DATA3 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const char* TEST_DATA4 = "zyxwvutsrqponmlkjihgfedcba";
	const char* TEST_DATA5 = "ZYXWVUTSRQPONMLKJIHGFEDCBA";
}


TEST_F( CompiledSpaceUnitTestHarness, BinaryContainerFormat_Empty )
{
	// Write out a test binary
	BinaryFormatWriter writer;
	CHECK( writer.numSections() == 0 );
	CHECK( writer.write( TEST_FILENAME ) == true );

	// Read back in and check data matches
	BinaryFormat reader;
	CHECK( reader.open( TEST_FILENAME ) );
	CHECK( reader.isValid() );
	CHECK( reader.numSections() == 0 );

	reader.close();
	CHECK( !reader.isValid() );
}

TEST_F( CompiledSpaceUnitTestHarness, BinaryContainerFormat_WithData )
{	
	// Write out a test binary
	BinaryFormatWriter writer;
	CHECK( writer.numSections() == 0 );

	BW::vector<unsigned char> testBigData;
	testBigData.resize( TEST_DATA1_LENGTH );
	for (size_t i = 0; i < TEST_DATA1_LENGTH; ++i)
	{
		testBigData[i] = rand() % 255;
	}

	BinaryFormatWriter::Stream* pStream1 = writer.appendSection( TEST_MAGIC1, TEST_VERSION1 );
	CHECK( pStream1 != NULL );
	CHECK( writer.numSections() == 1 );

	pStream1->writeRaw( (void*)&testBigData[0], TEST_DATA1_LENGTH );
	CHECK( pStream1->size() == TEST_DATA1_LENGTH );
	

	BinaryFormatWriter::Stream* pStream2 = writer.appendSection( TEST_MAGIC2, TEST_VERSION2 );
	CHECK( pStream2 != NULL );
	CHECK( writer.numSections() == 2 );

	pStream2->writeRaw( (void*)TEST_DATA2, strlen(TEST_DATA2) );
	CHECK( pStream2->size() == strlen(TEST_DATA2) );


	BinaryFormatWriter::Stream* pStream3 = writer.appendSection( TEST_MAGIC3, TEST_VERSION3 );
	CHECK( pStream3 != NULL );
	CHECK( writer.numSections() == 3 );

	pStream3->writeRaw( (void*)TEST_DATA3, strlen(TEST_DATA3) );
	CHECK( pStream3->size() == strlen(TEST_DATA3) );


	BinaryFormatWriter::Stream* pStream4 = writer.appendSection( TEST_MAGIC4, TEST_VERSION4 );
	CHECK( pStream4 != NULL );
	CHECK( writer.numSections() == 4 );

	pStream4->writeRaw( (void*)TEST_DATA4, strlen(TEST_DATA4) );
	CHECK( pStream4->size() == strlen(TEST_DATA4) );


	BinaryFormatWriter::Stream* pStream5 = writer.appendSection( TEST_MAGIC5, TEST_VERSION5 );
	CHECK( pStream5 != NULL );
	CHECK( writer.numSections() == 5 );

	pStream5->writeRaw( (void*)TEST_DATA5, strlen(TEST_DATA5) );
	CHECK( pStream5->size() == strlen(TEST_DATA5) );

	CHECK( writer.write( TEST_FILENAME ) == true );

	// Read back in and check data matches
	BinaryFormat reader;
	CHECK( reader.open( TEST_FILENAME ) );
	CHECK( reader.isValid() );
	CHECK( reader.numSections() == 5 );

	CHECK( reader.sectionMagic(0) == TEST_MAGIC1 );
	CHECK( reader.sectionMagic(1) == TEST_MAGIC2 );
	CHECK( reader.sectionMagic(2) == TEST_MAGIC3 );
	CHECK( reader.sectionMagic(3) == TEST_MAGIC4 );
	CHECK( reader.sectionMagic(4) == TEST_MAGIC5 );

	CHECK( reader.sectionVersion(0) == TEST_VERSION1 );
	CHECK( reader.sectionVersion(1) == TEST_VERSION2 );
	CHECK( reader.sectionVersion(2) == TEST_VERSION3 );
	CHECK( reader.sectionVersion(3) == TEST_VERSION4 );
	CHECK( reader.sectionVersion(4) == TEST_VERSION5 );

	CHECK( reader.sectionLength(0) == TEST_DATA1_LENGTH );
	CHECK( reader.sectionLength(1) == strlen(TEST_DATA2) );
	CHECK( reader.sectionLength(2) == strlen(TEST_DATA3) );
	CHECK( reader.sectionLength(3) == strlen(TEST_DATA4) );
	CHECK( reader.sectionLength(4) == strlen(TEST_DATA5) );

	{
		BinaryFormat::Stream* pStream1 = reader.openSection(0);
		CHECK( pStream1 != NULL );
		CHECK( memcmp( pStream1->readRaw(TEST_DATA1_LENGTH),
			(void*)&testBigData[0], TEST_DATA1_LENGTH ) == 0 );
		reader.closeSection( pStream1 );

		BinaryFormat::Stream* pStream2 = reader.openSection(1);
		CHECK( pStream2 != NULL );
		CHECK( memcmp( pStream2->readRaw( strlen(TEST_DATA2) ),
			(void*)TEST_DATA2, strlen(TEST_DATA2) ) == 0 );
		reader.closeSection( pStream2 );

		BinaryFormat::Stream* pStream3 = reader.openSection(2);
		CHECK( pStream3 != NULL );
		CHECK( memcmp( pStream3->readRaw( strlen(TEST_DATA3) ),
			(void*)TEST_DATA3, strlen(TEST_DATA3) ) == 0 );
		reader.closeSection( pStream3 );

		BinaryFormat::Stream* pStream4 = reader.openSection(3);
		CHECK( pStream4 != NULL );
		CHECK( memcmp( pStream4->readRaw( strlen(TEST_DATA4) ),
			(void*)TEST_DATA4, strlen(TEST_DATA4) ) == 0 );
		reader.closeSection( pStream4 );

		BinaryFormat::Stream* pStream5 = reader.openSection(4);
		CHECK( pStream5 != NULL );
		CHECK( memcmp( pStream5->readRaw( strlen(TEST_DATA5) ),
			(void*)TEST_DATA5, strlen(TEST_DATA5) ) == 0 );
		reader.closeSection( pStream5 );
	}

	{
		BinaryFormat::Stream* pStream1 = reader.openSection(0);
		BinaryFormat::Stream* pStream2 = reader.openSection(1);
		BinaryFormat::Stream* pStream3 = reader.openSection(2);
		BinaryFormat::Stream* pStream4 = reader.openSection(3);
		BinaryFormat::Stream* pStream5 = reader.openSection(4);
		
		CHECK( pStream1 != NULL );
		CHECK( pStream2 != NULL );
		CHECK( pStream3 != NULL );
		CHECK( pStream4 != NULL );
		CHECK( pStream5 != NULL );
		
		CHECK( memcmp( pStream1->readRaw(TEST_DATA1_LENGTH),
			(void*)&testBigData[0], TEST_DATA1_LENGTH ) == 0 );

		CHECK( memcmp( pStream2->readRaw( strlen(TEST_DATA2) ),
			(void*)TEST_DATA2, strlen(TEST_DATA2) ) == 0 );

		CHECK( memcmp( pStream3->readRaw( strlen(TEST_DATA3) ),
			(void*)TEST_DATA3, strlen(TEST_DATA3) ) == 0 );

		CHECK( memcmp( pStream4->readRaw( strlen(TEST_DATA4) ),
			(void*)TEST_DATA4, strlen(TEST_DATA4) ) == 0 );

		CHECK( memcmp( pStream5->readRaw( strlen(TEST_DATA5) ),
			(void*)TEST_DATA5, strlen(TEST_DATA5) ) == 0 );

		reader.closeSection( pStream1 );
		reader.closeSection( pStream2 );
		reader.closeSection( pStream3 );
		reader.closeSection( pStream4 );
		reader.closeSection( pStream5 );
	}

	reader.close();
	CHECK( !reader.isValid() );
}



TEST_F( CompiledSpaceUnitTestHarness, StringTable )
{
	// Write out test string table
	StringTableWriter inputTable;

	CHECK( inputTable.addString( "abcdefghijklmnopqrstuvwxyz" ) == 0 );
	CHECK( inputTable.addString( "!@#$\\%%^&*() qwertyuiop" ) == 1 );
	CHECK( inputTable.addString( "thing/with/path/separators/yeah" ) == 2 );
	CHECK( inputTable.addString( "A quick brown fox jumped over the lazy dog." ) == 3 );
	CHECK( inputTable.addString( "Herp derp derp." ) == 4 );

	for (size_t i = 0; i < 10000; ++i) // bunch of random strings
	{
		BW::string randomString;
		randomString.resize( rand() % 300 );

		for (size_t j = 0; j < randomString.size(); ++j)
		{
			randomString[j] = (char)(1 + rand() % 126);
		}

		inputTable.addString( randomString );
	}

	BinaryFormatWriter writer;
	CHECK( inputTable.write( writer ) );
	writer.write( "/test.bin" );

	// Read in and check against original data
	BinaryFormat reader;
	reader.open( "/test.bin" );
	CompiledSpace::StringTable table;
	CHECK( table.read( reader ) );
	CHECK( table.isValid() );

	CHECK( table.size() == inputTable.size() );

	for(size_t i = 0; i < table.size(); ++i)
	{
		CHECK( table.entryLength(i) == inputTable.entry(i).size() );
		CHECK( !strcmp( table.entry(i), inputTable.entry(i).c_str() ) );
	}

	table.close();
	CHECK( !table.isValid() );
}



TEST_F( CompiledSpaceUnitTestHarness, AssetList_Test )
{
	StringTableWriter stringTableWriter;
	AssetListWriter assetListWriter;
	CHECK( assetListWriter.size() == 0 );

	assetListWriter.addAsset( AssetListTypes::ASSET_TYPE_MODEL,
		"system/helpers/unit_cube.model", stringTableWriter );

	CHECK( assetListWriter.size() == 1 );

	assetListWriter.addAsset( AssetListTypes::ASSET_TYPE_TEXTURE,
		"a/b/blah.tga", stringTableWriter );

	CHECK( assetListWriter.size() == 2 );

	assetListWriter.addAsset( AssetListTypes::ASSET_TYPE_EFFECT,
		"shaders/std_effects/light_only.fx", stringTableWriter );

	CHECK( assetListWriter.size() == 3 );

	BinaryFormatWriter writer;
	CHECK( stringTableWriter.write( writer ) );
	CHECK( assetListWriter.write( writer ) );
	writer.write( "/test.bin" );

	StringTable stringTable;
	AssetList assetList;
	BinaryFormat reader;
	reader.open( "/test.bin" );

	CHECK( stringTable.read( reader ) );
	CHECK( !assetList.isValid() );
	CHECK( assetList.read( reader ) );
	CHECK( assetList.isValid() );
	CHECK( assetList.size() == assetListWriter.size() );

	for (size_t i = 0; i < assetList.size(); ++i)
	{
		CHECK( assetList.assetType(i) == assetListWriter.assetType(i) );
		CHECK( assetList.stringTableIndex(i) == i );

		BW::string srcResID = stringTableWriter.entry( assetListWriter.assetNameIndex( i ) );
		const char* resID = stringTable.entry( assetList.stringTableIndex(i) );
		CHECK( resID != NULL );
		CHECK( !strcmp( resID, srcResID.c_str() ) );
	}

	stringTable.close();
	assetList.close();
}


BW_END_NAMESPACE
