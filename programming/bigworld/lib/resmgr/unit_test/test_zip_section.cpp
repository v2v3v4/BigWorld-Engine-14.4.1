#include "pch.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/zip_section.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/multi_file_system.hpp"
#include "cstdmf/bwrandom.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "resmgr/zip_file_system.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/data_section_cache.hpp"
#ifdef WIN32	
#include <mmsystem.h>
#endif 

#define TEST_PRINT_NAME() BWUnitTest::unitTestInfo( "Running test: %s\n", m_name )


BW_BEGIN_NAMESPACE

//return a unique file name 
BW::string getUniqueFileName (const BW::string& fileName)
{
	size_t extensionPos = fileName.rfind(".");
	BW::string retFileName;
	BW::string fileExtension;
	if (extensionPos != BW::string::npos)
	{
		retFileName = fileName.substr(0, extensionPos);
		fileExtension = fileName.substr(extensionPos);
	}
	else
	{
		retFileName = fileName;
	}
#ifdef _DEBUG
	retFileName += "_Debug";
#else
	retFileName += "_Hybrid";
#endif
#ifdef _LP64
	retFileName += "64";
#else
	retFileName += "";
#endif
	retFileName += fileExtension;
	return retFileName;
}

namespace
{
	/**
	 *	This compares two memory blocks to see if they are exactly the same.
	 *
	 *	@param block1	The address of the first block.
	 *	@param block1Sz	The size of the first block.
	 *	@param block2	The address of the second block.
	 *	@param block2Sz	The size of the second block.
	 *  @return			True if the blocks are the same, false if they differ.
	 */
	bool compMemBlocks
	(
		void		const *block1,
		size_t		block1Sz,
		void		const *block2,
		size_t		block2Sz
	)
	{
		if (block1Sz != block2Sz)
			return false;
		return ::memcmp(block1, block2, block1Sz) == 0;
	}


	/**
	 *	This converts an object to a string representation.  It assumes that
	 *	operator<< is defined.
	 */
	template<typename T>
	BW::string toString(T const &t)
	{
		BW::stringstream out;
		out << t;
		return out.str();
	}


	/**
	 *	This is the number of files to write/read in a lots-of-files test.
	 */
	const size_t NUM_LOTS_OF_FILES = 2000;
}

/**
 *	This tests checks that a non-existant zip section does not actually exist.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_NonExistent )
{
	CHECK( this->isOK() );

	DataSectionPtr emptySection =
		BWResource::openSection("flying_cats");
	CHECK(!emptySection);
}


/**
 *	This tests whether an existant section exists.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_Existent )
{
	CHECK( this->isOK() );

	DataSectionPtr zipSection = BWResource::openSection("exist.zip");
	CHECK(zipSection);
}


/**
 *	This tests whether reading an empty section can be read and whether it
 *	is empty.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_Empty )
{
	CHECK( this->isOK() );

	DataSectionPtr zipSection = BWResource::openSection("empty.zip");
	CHECK(zipSection);
	CHECK(zipSection && zipSection->countChildren() == 0);
}


/**
 *	This opens an existing section and compares it with known values.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_ReadNumbers )
{
	CHECK( this->isOK() );

	DataSectionPtr zipSection = BWResource::openSection("numbers.zip");
	CHECK(zipSection);
	DataSectionPtr numSection = zipSection->openSection("child");
	CHECK(numSection);

	const int data[]    = { 4, 8, 15, 16, 23, 42 };
	BinaryPtr binData = numSection->asBinary();
	CHECK
	(
		compMemBlocks
		(
			&data[0]       , sizeof(data)  ,
			binData->data(), binData->len()
		)
	);
}


/**
 *	This tests writing and reading lots of files.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_LotsOfFiles )
{
	CHECK( this->isOK() );

	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();	
	for (size_t i = 0; i < NUM_LOTS_OF_FILES; ++i)
	{
		BW::string filename = 
			"lots_of_files/test_file_" + toString((int)i) + ".zip";		

		fileSystem->eraseFileOrDirectory( filename );
	}	

	BWRandom				random;
	BW::vector<BinaryPtr>	data;

	// Write out NUM_LOTS_OF_FILES files of random length between 512 and 1024
	// bytes with random data.
	for (size_t i = 0; i < NUM_LOTS_OF_FILES; ++i)
	{
		BW::string filename =
			"lots_of_files/test_file_" + toString((int)i) + ".zip";
		DataSectionPtr zipSection = BWResource::openSection("test", true,
										ZipSection::creator());
		CHECK(zipSection);
		DataSectionPtr child = zipSection->openSection("child", true,
									BinSection::creator());
		CHECK(child);
		BW::vector<uint8> buffer(random(512, 1024));
		if (buffer.size()>=2)
			buffer[0] = buffer[1] = 255;	
		for (size_t j = 2; j < buffer.size(); ++j)
		{
			uint8 val = (uint8)random( 0, 254 );
			if ( val == '<' )
				val = 255;
			buffer[j] = val;
		}
		BinaryPtr bin = new BinaryBlock(&buffer[0], buffer.size(), "unit_test");
		data.push_back(bin);
		bool ret = child->setBinary(bin);
		CHECK(ret);
		ret = zipSection->save(filename);
		CHECK(ret);
	}

	BWResource::instance().purgeAll();	// Clear any cached values

	// Read in the written files and compare the on-disk values with the values
	// in memory.
	for (size_t i = 0; i < NUM_LOTS_OF_FILES; ++i)
	{
		BW::string filename =
			"lots_of_files/test_file_" + toString((int)i) + ".zip";
		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK( zipSection.getObject() != NULL );
		if (!zipSection)
		{
			continue;
		}

		DataSectionPtr numSection = zipSection->openSection("child", false,
										BinSection::creator());
		CHECK( numSection.getObject() != NULL );
		if (!numSection)
		{
			continue;
		}

		BinaryPtr binData = numSection->asBinary();
		CHECK(binData);
		if (!binData)
		{
			continue;
		}

		CHECK
		(
			compMemBlocks
			(
				binData->data(), binData->len(),
				data[i]->data(), data[i]->len()
			)
		);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}

BinaryPtr randomBlock( BWRandom& random , uint32 size = 0)
{
	if (size == 0)
	{
		size = random(512, 1024);
	}
	BW::vector<uint8> buffer( size );
	for (size_t j = 0; j < buffer.size(); ++j)	
	{
		uint8 val = (uint8)random( 0, 254 );
		if ( val == '<' )
			val = 255;
		buffer[j] = val;
	}

	return new BinaryBlock( &buffer[0], buffer.size(), "unit_test" );
}

bool checkBlocks( BinaryPtr bin1, BinaryPtr bin2 )
{
	return compMemBlocks
	(
		bin1->data(), bin1->len(),
		bin2->data(), bin2->len()
	);
}

/**
 *	This tests creating one zip section, adding a child and reading it.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_TestCreate  )
{
	CHECK( this->isOK() );

	BWResource::instance().purgeAll();	// Clear any cached values	

	const uint32		duplicateCount	= 10;
	BWRandom			random;	
	BW::string			saveAsFilename	= "test_path/test2/test_create.zip";
	BW::string			testPath		= "test_path";
	BW::string			testTag			= "test_tag.zip";
	BW::string			filename		= testPath + "/" + testTag;
	BW::string			testString		= "test_string";
	BW::string			xmlSectionName	= "testSection";
	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();
	BW::vector<BinaryPtr>	data;
	BinaryPtr			origData;
	int					testInt = random(1, INT_MAX);
	bool				ret = false;

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( saveAsFilename );
	fileSystem->eraseFileOrDirectory( filename );

	///////////////////////////////////////////////////////////////////////////
	// Test Create
	{
		DataSectionPtr zipSection = BWResource::openSection( filename, true,
												ZipSection::creator() );
		CHECK(zipSection);
		CHECK(zipSection->sectionName() == testTag);
		CHECK(zipSection->countChildren() == 0); //make sure it wasnt loaded.

		//TODO: Add a packed section child

		// Add a bin section child
		DataSectionPtr binChild =
			zipSection->openSection( "binChild", true, BinSection::creator() );
		CHECK(binChild);
		origData = randomBlock(random);
		binChild->setBinary( origData );

		// Add a xml section child
		DataSectionPtr xmlChild =
			zipSection->openSection( "xmlChild", true, XMLSection::creator() );
		CHECK(xmlChild);
		CHECK(xmlChild->countChildren() == 0);

		DataSectionPtr testXML = xmlChild->openSection( xmlSectionName, true, XMLSection::creator() );
		ret = testXML->writeString( "testString", testString );
		CHECK(ret);
		ret = testXML->writeInt( "testInt", testInt );
		CHECK(ret);

		//TODO: Add a dir section child
		//DataSectionPtr dirChild =
		//	zipSection->openSection("dirChild", true, DirSection::creator());
		//CHECK(dirChild);

		// Add a nested zip section child
		DataSectionPtr zipChild =
			zipSection->openSection( "zipChild", true, ZipSection::creator() );
		CHECK(zipChild);

		zipChild = zipChild->openSection( "zipChild2", true, ZipSection::creator() );
		CHECK( zipChild );

		zipChild = zipChild->openSection( "zipChild3", true, ZipSection::creator() );
		CHECK( zipChild );


		// Add some sections with duplicate names
		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = zipChild->newSection( "nestedBin", BinSection::creator() );
			CHECK(nestedBin);
			BinaryPtr bin = randomBlock( random );
			data.push_back( bin );
			nestedBin->setBinary( bin );
		}
		CHECK(data.size() == duplicateCount);

		// write out empty binary data
		BinaryPtr empty = new BinaryBlock( "", 0, "test" );
		CHECK(zipSection->writeBinary( "empty", empty ));

		// Test regular save
		ret = zipSection->save();
		CHECK(ret);

		// Test save-as
		ret = zipSection->save(saveAsFilename);
		CHECK(ret);
	}

	///////////////////////////////////////////////////////////////////////////
	// Test Load

	// Load and check the values.
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Read in the written files and compare the on-disk values with the values
		// in memory.
		DataSectionPtr saveAsSection = BWResource::openSection(saveAsFilename);
		CHECK(saveAsSection);

		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK(zipSection);

		// verify the empty binary data we wrote
		BinaryPtr empty = zipSection->readBinary( "empty" );
		CHECK(empty.hasObject() && empty->len() == 0);

		// Load the bin section child
		//DataSectionPtr binChild = zipSection->openSection( "binChild", false, BinSection::creator() );
		DataSectionPtr binChild = zipSection->openSection( "binChild" );
		CHECK(binChild);		
		BinaryPtr binData = binChild->asBinary();
		CHECK(binData);
		CHECK(checkBlocks(binData, origData));

		// Load the xml section child
		DataSectionPtr xmlChild = zipSection->openSection( "xmlChild" );
		CHECK(xmlChild);

		DataSectionPtr testXML = xmlChild->openSection( xmlSectionName );
		CHECK(testXML);
		BW::string inString = testXML->readString( "testString", "" );
		CHECK(inString == testString);		
		int inVal = testXML->readInt( "testInt", 0 );
		CHECK(inVal == testInt);

		//TODO: Load the dir section child
		//DataSectionPtr dirChild =
		//	zipSection->openSection( "dirChild" );
		//CHECK(dirChild);

		// Load the nested zip section child
		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);

		zipChild = zipChild->openSection( "zipChild2/zipChild3" );
		CHECK(zipChild);

		CHECK(zipChild->countChildren());

		// Test the duplicate name sections
		BW::vector<DataSectionPtr> sections;
		zipChild->openSections( "nestedBin", sections, BinSection::creator() );
		CHECK(sections.size() == duplicateCount);
		CHECK(data.size() == duplicateCount);

		//TODO: is creation order meant to be guaranteed with sections of the same name?
		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = sections[i];
			CHECK(nestedBin);
			BinaryPtr bin = nestedBin->asBinary();
			CHECK(bin);
			CHECK(checkBlocks(bin, data[i]));
		}

		zipChild->deleteSections( "nestedBin" );
		CHECK(zipChild->countChildren() == 0);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}


bool checkChildExists(DataSectionPtr pSection, const BW::string& sectionName)
{
	bool found = false;
	if (pSection)
	{
		for (uint i=0; i<(uint)pSection->countChildren() && !found; i++)
		{
			found = (sectionName == pSection->childSectionName(i));
		}
	}
	return found;
}

TEST_F( ResMgrUnitTestHarness, ZipSection_TestDelete )
{
	CHECK( this->isOK() );

	BWResource::instance().purgeAll();	// Clear any cached values	

	const uint32		duplicateCount	= 10;
	BWRandom			random;	
	BW::string			emptyName		= "test_deleted.zip";
	BW::string			testPath		= "test_path";
	BW::string			testTag			= "test_delete.zip";
	BW::string			filename		= testPath + "/" + testTag;
	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();
	BW::vector<BinaryPtr>	data;
	BinaryPtr			origData;
	bool				ret = false;

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( emptyName );
	fileSystem->eraseFileOrDirectory( filename );
	{
		DataSectionPtr zipSection = BWResource::openSection( filename, true,
												ZipSection::creator() );
		CHECK(zipSection);
		CHECK(zipSection->sectionName() == testTag);
		CHECK(zipSection->countChildren() == 0); //make sure it wasnt loaded.

		// Add a nested zip section child
		DataSectionPtr zipChild =
			zipSection->openSection( "zipChild", true, ZipSection::creator() );
		CHECK(zipChild);

		DataSectionPtr child1 = zipChild->newSection( "child1", BinSection::creator() );
		CHECK(child1);
		BinaryPtr bin = randomBlock( random );
		child1->setBinary( bin );
		data.push_back(bin);

		DataSectionPtr child2 = zipChild->newSection( "child2", BinSection::creator() );
		CHECK(child2);
		bin = randomBlock( random );
		child2->setBinary( bin );
		data.push_back(bin);

		DataSectionPtr child3 = zipChild->newSection( "child3", BinSection::creator() );
		CHECK(child3);
		bin = randomBlock( random );
		child3->setBinary( bin );
		data.push_back(bin);

		// Add some sections with duplicate names
		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = zipChild->newSection( "nestedBin", BinSection::creator() );
			CHECK(nestedBin);
			BinaryPtr bin = randomBlock( random );
			nestedBin->setBinary( bin );
		}

		// regular save
		ret = zipSection->save();
		CHECK(ret);
	}

	// Load and check the values.
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Read in the written files and compare the on-disk values with the values
		// in memory.
		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK(zipSection);

		// Load the nested zip section child
		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);
		CHECK(zipChild->countChildren() == (duplicateCount+3));

		DataSectionPtr child1 = zipChild->openSection("child1");
		CHECK(child1);
		CHECK( zipChild->deleteSection( child1->sectionName() ) );
		CHECK( zipChild->deleteSection( "child2" ) );
		CHECK( zipChild->deleteSection( "child3" ) );

		zipChild->deleteSections( "nestedBin" );
		CHECK(zipChild->countChildren() == 0);

		// add a single child
		DataSectionPtr pChild = zipChild->newSection( 
								"pChild", BinSection::creator() );
		CHECK(pChild);
		BinaryPtr bin = randomBlock( random );
		pChild->setBinary( bin );
		data.push_back(bin);

		CHECK(zipSection->save(emptyName));
	}

	// Load again to check the saved file is empty
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(emptyName);
		CHECK(zipSection);

		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);
		CHECK(zipChild->countChildren() == 1);

		DataSectionPtr pChild = zipChild->openSection( "pChild" );
		CHECK(pChild);
		BinaryPtr bin = pChild->asBinary();
		CHECK(bin);
		CHECK(checkBlocks(bin, data[3]));
	}

	// A test very similar to the terrain data saving process currently:
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr cDataDS = BWResource::openSection( "blank.cdata" );
		CHECK(cDataDS);
		//if ( cDataDS )
		{
			DataSectionPtr blockDS = cDataDS->openSection( "terrain2" );
			CHECK(blockDS);
			uint childCount = blockDS->countChildren();
			//if ( blockDS )
			{
				{
					blockDS->deleteSections( "heights" ); //only one
					CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
					CHECK(!checkChildExists(blockDS, "heights"));
					{
						DataSectionPtr pSection = blockDS->newSection(
											"heights", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(((uint32)blockDS->countChildren()) == childCount);
					CHECK(checkChildExists(blockDS, "heights"));
				}

				{
					blockDS->deleteSections( "normals" );
					CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
					CHECK(!checkChildExists(blockDS, "normals"));

					blockDS->deleteSections( "lodNormals" );
					CHECK(((uint32)blockDS->countChildren()) == (childCount-2));
					CHECK(!checkChildExists(blockDS, "lodNormals"));
					{
						DataSectionPtr pSection = blockDS->newSection(
											"normals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
					CHECK(checkChildExists(blockDS, "normals"));

					{
						DataSectionPtr pSection = blockDS->newSection(
										"lodNormals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(((uint32)blockDS->countChildren()) == childCount);
					CHECK(checkChildExists(blockDS, "lodNormals"));
				}

				blockDS->deleteSections( "horizonShadows" );
				CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
				CHECK(!checkChildExists(blockDS, "horizonShadows"));

				{
					DataSectionPtr pSection = blockDS->newSection(
									"horizonShadows", BinSection::creator());
					CHECK(pSection);
					BinaryPtr bin = randomBlock( random );
					pSection->setBinary( bin );
				}
				CHECK(((uint32)blockDS->countChildren()) == childCount);
				CHECK(checkChildExists(blockDS, "horizonShadows"));
				CHECK( cDataDS->save("delete_result.cdata") );
			}
		}
	}

	// A test very similar to the terrain data saving process currently:
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr cDataDS = BWResource::openSection( "testA.cdata",
													true,
													ZipSection::creator() );
		CHECK(cDataDS);
		//if ( cDataDS )
		{
			DataSectionPtr blockDS = cDataDS->newSection( "terrain2",
													ZipSection::creator());
			CHECK(blockDS);
			uint childCount = blockDS->countChildren();
			CHECK(childCount==0);
			//if ( blockDS )
			{
				{
					blockDS->deleteSections( "heights" ); //only one
					CHECK(!checkChildExists(blockDS, "heights"));
					{
						DataSectionPtr pSection = blockDS->newSection("heights",
														BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(checkChildExists(blockDS, "heights"));
				}

				{
					blockDS->deleteSections( "normals" );
					CHECK(!checkChildExists(blockDS, "normals"));

					blockDS->deleteSections( "lodNormals" );
					CHECK(!checkChildExists(blockDS, "lodNormals"));
					{
						DataSectionPtr pSection = blockDS->newSection("normals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(checkChildExists(blockDS, "normals"));
					{
						DataSectionPtr pSection = blockDS->newSection("lodNormals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(checkChildExists(blockDS, "lodNormals"));
				}

				blockDS->deleteSections( "horizonShadows" );
				CHECK(!checkChildExists(blockDS, "horizonShadows"));
				{
					DataSectionPtr pSection = blockDS->newSection("horizonShadows", BinSection::creator());
					CHECK(pSection);
					BinaryPtr bin = randomBlock( random );
					pSection->setBinary( bin );
				}
				CHECK(checkChildExists(blockDS, "horizonShadows"));				
			}
		}		
		DataSectionPtr pSection = cDataDS->newSection("bin1", BinSection::creator());
		CHECK(pSection);
		BinaryPtr bin = randomBlock( random );
		pSection->setBinary( bin );

		CHECK( cDataDS->save("delete_and_save_result.cdata") );
	}


	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr cDataDS = BWResource::openSection( "delete_and_save_result.cdata" );
		CHECK(cDataDS);
		//if ( cDataDS )
		{
			DataSectionPtr blockDS = BWResource::openSection( "delete_and_save_result.cdata/terrain2" );
			CHECK(blockDS);
			//if ( blockDS )
			{
				CHECK(checkChildExists(blockDS, "heights"));
				CHECK(checkChildExists(blockDS, "normals"));
				CHECK(checkChildExists(blockDS, "lodNormals"));
				CHECK(checkChildExists(blockDS, "horizonShadows"));				
			}
		}
		CHECK(checkChildExists(cDataDS, "bin1"));
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}


TEST_F( ResMgrUnitTestHarness, ZipSection_TestConversions )
{
	CHECK( this->isOK() );

	BWRandom			random;
	BW::string			saveAsFilename	= "test_conversion.zip";
	BW::string			saveAsFilename2	= "test_conversion2.zip";
	bool				ret = false;
	BinaryPtr			origData;

	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( saveAsFilename );
	fileSystem->eraseFileOrDirectory( saveAsFilename2 );

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Load, modify and save a zip file with directories
		DataSectionPtr binSection = BWResource::openSection( "test_bin.bin",
										true, BinSection::creator() );
		CHECK(binSection);

		DataSectionPtr pTest = binSection->newSection( "test",
									BinSection::creator() );
		CHECK(pTest);

		pTest->deleteSections( "binChild" );
		DataSectionPtr binChild = pTest->newSection( "binChild" );
		CHECK(binChild);	
		origData = randomBlock(random);
		binChild->setBinary( origData );	

		DataSectionPtr zipSection = binSection->convertToZip();
		CHECK(zipSection);
		CHECK(zipSection.getObject() != binSection.getObject());

		ret = zipSection->save(saveAsFilename);
		CHECK(ret);

		ret = binChild->save(saveAsFilename2);
		CHECK(ret);

		DataSectionPtr pChild = zipSection->openSection( "test" );
		CHECK(pChild);

		// if this is already a zip, it will return itself.
		DataSectionPtr pTestChild = pChild->convertToZip();
		CHECK(pTestChild);
		CHECK(pTestChild.getObject() == pChild.getObject());
	}

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(saveAsFilename);
		CHECK(zipSection);
		DataSectionPtr binSection = BWResource::openSection(saveAsFilename2);
		CHECK(binSection);
		DataSectionPtr pChild = zipSection->openSection( "test" );
		CHECK(pChild);
		DataSectionPtr binChild = pChild->openSection( "binChild" );
		CHECK(binChild);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// load a binary section as if it were a zip section
		DataSectionPtr binSection = BWResource::openSection( "test_binary",
										true, ZipSection::creator() );
		CHECK(binSection);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}

TEST_F( ResMgrUnitTestHarness, ZipSection_TestNestedDirectory )
{
	CHECK( this->isOK() );

	BWRandom			random;
	BW::string			saveAsFilename	= "test_path/test2/test_dir_result.zip";
	BW::string			testString		= "test_string";
	BW::string			xmlSectionName	= "testSection";
	int					testInt = random(1, INT_MAX);
	bool				ret = false;
	const uint32		duplicateCount	= 10;
	BinaryPtr			origData;

	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( saveAsFilename );

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Load, modify and save a zip file with directories
		DataSectionPtr zipSection = BWResource::openSection( "test_path/test_dir.zip" );
		CHECK(zipSection);

		// Load the nested zip section child
		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);

		// Test the duplicate name sections
		BW::vector<DataSectionPtr> sections;
		zipChild->openSections( "nestedBin", sections, BinSection::creator() );
		CHECK(sections.size() == duplicateCount);

		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = sections[i];
			CHECK(nestedBin);
			BinaryPtr bin = nestedBin->asBinary();
			CHECK(bin);
		}

		DataSectionPtr nestedXMLChild =
			zipSection->openSection( "zipChild/xmlChild", true, XMLSection::creator() );
		CHECK(nestedXMLChild);
		CHECK(nestedXMLChild->countChildren() == 0);

		DataSectionPtr testXML2 = nestedXMLChild->openSection( xmlSectionName, true, XMLSection::creator() );
		CHECK(testXML2);
		ret = testXML2->writeString( "testString", testString );
		CHECK(ret);
		ret = testXML2->writeInt( "testInt", testInt );
		CHECK(ret);

		ret = zipSection->save(saveAsFilename);
		CHECK(ret);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(saveAsFilename);
		CHECK(zipSection);

		DataSectionPtr nestedXMLChild = zipSection->openSection( "zipChild/xmlChild" );
		CHECK(nestedXMLChild);

		DataSectionPtr testXML = nestedXMLChild->openSection( xmlSectionName );
		CHECK(testXML);
		BW::string test = testXML->readString( "testString", "" );
		CHECK(test == testString);
		int test2 = testXML->readInt( "testInt", 0 );
		CHECK( test2 == testInt );
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}

/**
 *	This tests a particular case where a zip was loading the same child 
 *	section twice, causing a crash when you try to delete the section.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_DupChild )
{
	CHECK( this->isOK() );

	BW::string filename = "dup_child_test.zip";
	BW::string saveFilename = "dup_child_test_save.zip";

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK(zipSection);
		CHECK(zipSection->findChild("lighting"));

		zipSection->delChild("lighting");

		CHECK(!zipSection->findChild("lighting"));
		CHECK(zipSection->save(saveFilename));
	}

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(saveFilename);
		CHECK(zipSection);
		CHECK(!zipSection->findChild("lighting"));		
	}
}

/**
 *	This tests creating one zip section, adding a multiple childs and reading them.
 *  In addition it testes using multiple res paths
 *  Was used to debug and understand performance issues when using multiple res paths (some zipped)
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_TestCreate_And_Read_Specific_Case  )
{
	CHECK( this->isOK() );
#ifdef WIN32	
	DWORD startTime = timeGetTime();
#endif
	BWResource::instance().purgeAll();	// Clear any cached values	

	const uint32		childCount	= 300;
	BWRandom			random;	
	BW::string			saveAsFilename	= "test_path/test2/test_create1.zip";
	BW::string			testPath		= "test_path";
	BW::string			testTag			= getUniqueFileName("test_tag.zip");
	BW::string			filename		= testPath + "/" + testTag;
	BW::string			testString		= "test_string";
	BW::string			xmlSectionName	= "testSection";
	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();
	BW::vector<BinaryPtr>	data;
	BinaryPtr			origData;
	int					testInt = random(1, INT_MAX);
	bool				ret = false;

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( saveAsFilename );
	fileSystem->eraseFileOrDirectory( filename );

	///////////////////////////////////////////////////////////////////////////
	// Test Create
	{
		DataSectionPtr zipSection = BWResource::openSection( filename, true,
												ZipSection::creator() );
		CHECK(zipSection);
		CHECK(zipSection->sectionName() == testTag);
		CHECK(zipSection->countChildren() == 0); //make sure it wasnt loaded.

		// Add a bin section child
		DataSectionPtr binChild =
			zipSection->openSection( "binChild", true, BinSection::creator() );
		CHECK(binChild);
		origData = randomBlock(random);
		binChild->setBinary( origData );

		// Add a xml section child
		DataSectionPtr xmlChild =
			zipSection->openSection( "xmlChild", true, XMLSection::creator() );
		CHECK(xmlChild);
		CHECK(xmlChild->countChildren() == 0);

		DataSectionPtr testXML = xmlChild->openSection( xmlSectionName, true, XMLSection::creator() );
		ret = testXML->writeString( "testString", testString );
		CHECK(ret);
		ret = testXML->writeInt( "testInt", testInt );
		CHECK(ret);

		// Add a nested zip section child
		DataSectionPtr zipChild =
			zipSection->openSection( "zipChild", true, ZipSection::creator() );
		CHECK(zipChild);

		zipChild = zipChild->openSection( "zipChild2", true, ZipSection::creator() );
		CHECK( zipChild );

		zipChild = zipChild->openSection( "zipChild3", true, ZipSection::creator() );
		CHECK( zipChild );


		// Add some childs
		for (size_t i=0; i<childCount; i++)
		{
			char buf[200];
			sprintf( buf, "child%" PRIzd, i );
			DataSectionPtr nestedBin = zipChild->newSection( buf, BinSection::creator() );
			CHECK(nestedBin);
			BinaryPtr bin = randomBlock( random, 40000 );
			data.push_back( bin );
			nestedBin->setBinary( bin );
		}
		CHECK(data.size() == childCount);

		// write out empty binary data
		BinaryPtr empty = new BinaryBlock( "", 0, "test" );
		CHECK(zipSection->writeBinary( "empty", empty ));

		// Test regular save
		ret = zipSection->save();
		CHECK(ret);

		// Test save-as
		ret = zipSection->save(saveAsFilename);
		CHECK(ret);
	}

#ifdef WIN32	
	DWORD writeEndTime = timeGetTime();
	BWUnitTest::unitTestInfo("Elapsed write Time %lu\n", (writeEndTime - startTime));
#endif
	///////////////////////////////////////////////////////////////////////////
	// Test Load
	// Load and check the values.
	//store the original res path
	BW::vector<BW::string> originalPaths;
	for (int i = 0; i < BWResource::instance().getPathNum(); ++i)
	{
		originalPaths.push_back( BWResource::instance().getPath(i) );
	}
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Read in the written files and compare the on-disk values with the values
		// in memory.
		DataSectionPtr saveAsSection = BWResource::openSection(saveAsFilename);
		CHECK(saveAsSection);

		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK(zipSection);

		// verify the empty binary data we wrote
		BinaryPtr empty = zipSection->readBinary( "empty" );
		CHECK(empty.hasObject() && empty->len() == 0);

		// Load the bin section child
		//DataSectionPtr binChild = zipSection->openSection( "binChild", false, BinSection::creator() );
		DataSectionPtr binChild = zipSection->openSection( "binChild" );
		CHECK(binChild);		
		BinaryPtr binData = binChild->asBinary();
		CHECK(binData);
		CHECK(checkBlocks(binData, origData));

		// Load the xml section child
		DataSectionPtr xmlChild = zipSection->openSection( "xmlChild" );
		CHECK(xmlChild);

		DataSectionPtr testXML = xmlChild->openSection( xmlSectionName );
		CHECK(testXML);
		BW::string inString = testXML->readString( "testString", "" );
		CHECK(inString == testString);		
		int inVal = testXML->readInt( "testInt", 0 );
		CHECK(inVal == testInt);

		// Load the nested zip section child
		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);

		zipChild = zipChild->openSection( "zipChild2/zipChild3" );
		CHECK(zipChild);

		CHECK(zipChild->countChildren());

		//change the res path to allow testing mutliple paths
		BW::string currPath = BWResource::instance().getPath(0);
		BW::string addedPath = currPath + "/" + saveAsFilename;
		BWResource::instance().delPath(0);
		BW::string paths = addedPath;
		paths +=  BW_RES_PATH_SEPARATOR;
		paths += currPath;
		paths += BW_RES_PATH_SEPARATOR;
		paths += addedPath;
		BWResource::instance().addPaths( paths, "More Paths", "" );

		for (size_t i=0; i<childCount; i++)
		{
			char buf[200];
			sprintf( buf, "child%" PRIzd, i );
			DataSectionPtr nestedBin = zipChild->openSection( buf );

			CHECK(nestedBin);
			BinaryPtr bin = nestedBin->asBinary();
			CHECK(bin);
			CHECK(checkBlocks(bin, data[i]));


			//ZipFileSystemPtr temp = new ZipFileSystem(saveAsFilename);
			//do 50 misses - make sure they don't cause performance issues
			for (int i = 0; i < 50; i++)
			{
				DataSectionCensus::clear();
				DataSectionCache::instance()->clear();
				BW::string name = "test_deleted.zip";
				DataSectionPtr temp = BWResource::instance().openSection( name );
			}
		}
	}
#ifdef WIN32	
	DWORD readEndTime = timeGetTime();
	BWUnitTest::unitTestInfo("Elapsed read Time %lu\n", (readEndTime - writeEndTime));
#endif 
	BWResource::instance().purgeAll();	// Clear any cached values
	while (BWResource::instance().getPathNum() > 0)
	{
		BWResource::instance().delPath(0);
	}
	//revert the original res path
	for (unsigned int i = 0; i < originalPaths.size(); ++i)
	{
		BWResource::instance().addPath( originalPaths[i] );
	}
}

/**
 *	This tests a particular case where a zip contains empty folders,
 *	causing a crash when you try to save it by using ZipFileSystem.
 */
TEST_F( ResMgrUnitTestHarness, ZipSection_TestEmptySubFolder )
{
	CHECK( this->isOK() );

	BWRandom			random;
	BW::string			saveAsFilename	= "empty_subfolder.zip";
	DataSectionPtr zipSection = BWResource::openSection( saveAsFilename, true,
		ZipSection::creator() );
	saveAsFilename = saveAsFilename + "_tmp";
	zipSection->save(saveAsFilename);
	
	CHECK( BWResource::resolveToAbsolutePath( saveAsFilename ) != IFileSystem::FT_NOT_FOUND);
	ZipFileSystemPtr fileSystem = new ZipFileSystem( saveAsFilename );

	BWResource::instance().purgeAll();
	DataSectionPtr xmlSection = BWResource::openSection( "test_xml_section.xml" );

	fileSystem->writeFile( "test_xml_section.xml", xmlSection->asBinary(), true );

	BWResource::instance().purgeAll();
	DataSectionPtr orgSection = BWResource::openSection( "test_xml_section.xml" );
	DataSectionPtr savedSection = BWResource::openSection( "empty_subfolder.zip_tmp/test_xml_section.xml" );
	CHECK( 
		checkBlocks( orgSection->asBinary(), savedSection->asBinary() )
		);

	BWResource::instance().fileSystem()->eraseFileOrDirectory( saveAsFilename );
}


/**
 *	This tests a particular case where a zip contains another zip file,
 *	causing a folder in the contained zip file becomes an empty binary after save.
 */
/* This test fails on server site, I disabled it for the server guys to investigate further.
 * I created bug BWT-22538 for the issue.
 * Will enable this test case again once everything is stable again.
TEST_F( ResMgrUnitTestHarness, ZipSection_TestZipInZip )
{
	CHECK( this->isOK() );

	BWRandom			random;
	BW::string			saveAsFilename	= "zip_in_zip.zip";
	
	CHECK( BWResource::resolveToAbsolutePath( saveAsFilename ) != IFileSystem::FT_NOT_FOUND);
	ZipFileSystemPtr fileSystem = new ZipFileSystem( saveAsFilename );

	BWResource::instance().purgeAll();


	DirSection* pDirSection = new DirSection( "folder1",
		fileSystem.getObject() );
	pDirSection->tag( "folder1" );
	pDirSection->save();


	fileSystem = new ZipFileSystem( fileSystem, "folder1/folder1_1/a_zip_file.zip" );

	IFileSystem::FileType ft1 = fileSystem->getFileTypeEx( "not_empty_folder" );
	//todo: This test failed with empty folder, bug BWT-22513 is created for this issue.
	//IFileSystem::FileType ft2 = fileSystem->getFileTypeEx( "empty_folder" );

	CHECK(ft1 == IFileSystem::FT_DIRECTORY);
	//CHECK(ft2 == IFileSystem::FT_DIRECTORY);

	bw_safe_delete( pDirSection );

//	fileSystem->writeFile( "zip_in_zip.zip/folder1/folder1_1/a_zip_file.zip", zipFileSection->asBinary(), true );
}
*/
BW_END_NAMESPACE

// test_zip_section.cpp
