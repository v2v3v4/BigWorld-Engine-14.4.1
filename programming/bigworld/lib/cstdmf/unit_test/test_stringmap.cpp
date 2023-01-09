#include "pch.hpp"

#include "cstdmf/stringmap.hpp"

BW_BEGIN_NAMESPACE

TEST( StringMap_empty_clear )
{
	StringMap<int> constStringmap;
	// Initial values are correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
	// Clear on empty
	constStringmap.clear();
	// Initial values are still correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
}


TEST( StringMap_insert_and_clear )
{
	StringMap<int> constStringmap;
	// Initial values are correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
	// Insert values
	CHECK( constStringmap.insert( std::make_pair( "test", 1 ) ).second );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	CHECK( constStringmap.insert( std::make_pair( "test2", 2 ) ).second );
	CHECK( constStringmap.size() == 2 );
	CHECK( !constStringmap.empty() );
	// Clear on empty
	constStringmap.clear();
	// Initial values are still correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
}


TEST( StringMap_insert_and_erase_by_key )
{
	StringMap<int> constStringmap;
	// Initial values are correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
	// Insert values
	CHECK( constStringmap.insert( std::make_pair( "test", 1 ) ).second );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	CHECK( constStringmap.insert( std::make_pair( "test2", 2 ) ).second );
	CHECK( constStringmap.size() == 2 );
	CHECK( !constStringmap.empty() );
	// Erase non-existant
	CHECK( constStringmap.erase( "test3" ) == 0 );
	CHECK( constStringmap.size() == 2 );
	CHECK( !constStringmap.empty() );
	// Erase existing
	CHECK( constStringmap.erase( "test" ) == 1 );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	// Erase last item
	CHECK( constStringmap.erase( "test2" ) == 1 );
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
	// Erase last item again
	CHECK( constStringmap.erase( "test2" ) == 0 );
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
}


TEST( StringMap_insert_and_erase_by_iter )
{
	StringMap<int> constStringmap;
	// Initial values are correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
	// Insert values
	CHECK( constStringmap.insert( std::make_pair( "test", 1 ) ).second );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	CHECK( constStringmap.insert( std::make_pair( "test2", 2 ) ).second );
	CHECK( constStringmap.size() == 2 );
	CHECK( !constStringmap.empty() );
	// Erase test2
	StringMap<int>::iterator it = constStringmap.find( "test2" );
	constStringmap.erase( it );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	// Check that the only one left is test1
	it = constStringmap.find( "test" );
	CHECK( it == constStringmap.begin() );
	CHECK( it != constStringmap.end() );
	constStringmap.erase( it );
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
}


TEST( StringMap_insert_and_iter )
{
	StringMap<int> constStringmap;
	// Initial values are correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
	// Insert values
	CHECK( constStringmap.insert( std::make_pair( "test", 1 ) ).second );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	CHECK( constStringmap.insert( std::make_pair( "test2", 2 ) ).second );
	CHECK( constStringmap.size() == 2 );
	CHECK( !constStringmap.empty() );
	// Erase test2
	StringMap<int>::iterator it = constStringmap.begin();
	CHECK( it != constStringmap.end() );
	++it;
	CHECK( it != constStringmap.end() );
	++it;
	CHECK( it == constStringmap.end() );
}


TEST( StringMap_bracket_operator )
{
	StringMap<int> constStringmap;
	// Initial values are correct
	CHECK( constStringmap.size() == 0 );
	CHECK( constStringmap.empty() );
	// Insert values
	constStringmap["test"] = 1;
	CHECK( constStringmap.size() == 1 );
	CHECK( constStringmap["test"] == 1 );
	CHECK( !constStringmap.empty() );
	
	constStringmap["test2"] = 2;
	CHECK( constStringmap.size() == 2 );
	CHECK( constStringmap["test2"] == 2 );
	CHECK( !constStringmap.empty() );
	
	// Update value
	constStringmap["test2"] = 3;
	CHECK( constStringmap["test2"] == 3 );
	CHECK( constStringmap.size() == 2 );
	CHECK( !constStringmap.empty() );
	
	// Update value
	StringMap<int>::iterator it = constStringmap.find("test2");
	CHECK( it != constStringmap.end() );
	CHECK( strcmp(it->first, "test2") == 0 );
	CHECK( it->second == 3 );
}


TEST( StringMap_const_policy )
{
	typedef StringMap<int, ConstPolicy> MapType;
	MapType constStringmap;
	char * test = "test";
	char * test2 = "test2";
	
	// Initial values are correct
	std::pair<MapType::iterator, bool> result = 
		constStringmap.insert( std::make_pair( test, 1 ) );
	CHECK( result.second );
	CHECK( result.first->first == test );
	CHECK( result.first->second == 1 );

	// Initial values are correct
	result = constStringmap.insert( std::make_pair( test2, 2 ) );
	CHECK( result.second );
	CHECK( result.first->first == test2 );
	CHECK( result.first->second == 2 );
	
	// Erase values
	CHECK( constStringmap.erase( "test" ) );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	
	MapType::iterator it = constStringmap.find( "test2" );
	CHECK( it != constStringmap.end() );
	CHECK( it->first == test2 );
	CHECK( it->second == 2 );
}


TEST( StringMap_strdup_policy )
{
	typedef StringMap<int, StrdupPolicy> MapType;
	MapType constStringmap;
	char * test = "test";
	char * test2 = "test2";
	
	// Initial values are correct
	std::pair<MapType::iterator, bool> result = 
		constStringmap.insert( std::make_pair( test, 1 ) );
	CHECK( result.second );
	CHECK( result.first->first != test );
	CHECK( strcmp( result.first->first, test ) == 0 );
	CHECK( result.first->second == 1 );

	// Initial values are correct
	result = constStringmap.insert( std::make_pair( test2, 2 ) );
	CHECK( result.second );
	CHECK( result.first->first != test2 );
	CHECK( strcmp( result.first->first, test2 ) == 0 );
	CHECK( result.first->second == 2 );
	
	// Erase values
	CHECK( constStringmap.erase( "test" ) );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	
	MapType::iterator it = constStringmap.find( "test2" );
	CHECK( it != constStringmap.end() );
	CHECK( it->first != test2 );
	CHECK( strcmp( it->first, test2 ) == 0 );
	CHECK( it->second == 2 );
}

TEST( StringMap_free_policy )
{
	typedef StringMap<int, FreePolicy> MapType;
	MapType constStringmap;
	char * test = (char*)bw_malloc(16);
	strncpy(test, "test", 16);
	char * test2 = (char*)bw_malloc(16);
	strncpy(test2, "test2", 16);
	
	// Initial values are correct
	std::pair<MapType::iterator, bool> result = 
		constStringmap.insert( std::make_pair( test, 1 ) );
	CHECK( result.second );
	CHECK( result.first->first == test );
	CHECK( strcmp( result.first->first, test ) == 0 );
	CHECK( result.first->second == 1 );

	// Initial values are correct
	result = constStringmap.insert( std::make_pair( test2, 2 ) );
	CHECK( result.second );
	CHECK( result.first->first == test2 );
	CHECK( strcmp( result.first->first, "test2" ) == 0 );
	CHECK( result.first->second == 2 );
	
	// Erase values
	CHECK( constStringmap.erase( "test" ) );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	
	MapType::iterator it = constStringmap.find( "test2" );
	CHECK( it != constStringmap.end() );
	CHECK( it->first == test2 );
	CHECK( strcmp( it->first, "test2" ) == 0 );
	CHECK( it->second == 2 );
}


TEST( StringMap_delete_policy )
{
	typedef StringMap<int, FreePolicy> MapType;
	MapType constStringmap;
	char * test = new char[16];
	strncpy(test, "test", 16);
	char * test2 = new char[16];
	strncpy(test2, "test2", 16);
	
	// Initial values are correct
	std::pair<MapType::iterator, bool> result = 
		constStringmap.insert( std::make_pair( test, 1 ) );
	CHECK( result.second );
	CHECK( result.first->first == test );
	CHECK( strcmp( result.first->first, "test" ) == 0 );
	CHECK( result.first->second == 1 );

	// Initial values are correct
	result = constStringmap.insert( std::make_pair( test2, 2 ) );
	CHECK( result.second );
	CHECK( result.first->first == test2 );
	CHECK( strcmp( result.first->first, "test2" ) == 0 );
	CHECK( result.first->second == 2 );
	
	// Erase values
	CHECK( constStringmap.erase( "test" ) );
	CHECK( constStringmap.size() == 1 );
	CHECK( !constStringmap.empty() );
	
	MapType::iterator it = constStringmap.find( "test2" );
	CHECK( it != constStringmap.end() );
	CHECK( it->first == test2 );
	CHECK( strcmp( it->first, "test2" ) == 0 );
	CHECK( it->second == 2 );
}

//
// StringSet tests
//

TEST( StringSet_empty_clear )
{
	StringSet<> constStringSet;
	// Initial values are correct
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
	// Clear on empty
	constStringSet.clear();
	// Initial values are still correct
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
}


TEST( StringSet_insert_and_clear )
{
	StringSet<> constStringSet;
	// Initial values are correct
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
	// Insert values
	CHECK( constStringSet.insert( "test" ).second );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	CHECK( constStringSet.insert( "test2" ).second );
	CHECK( constStringSet.size() == 2 );
	CHECK( !constStringSet.empty() );
	// Clear on empty
	constStringSet.clear();
	// Initial values are still correct
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
}


TEST( StringSet_insert_and_erase_by_key )
{
	StringSet<> constStringSet;
	// Initial values are correct
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
	// Insert values
	CHECK( constStringSet.insert( "test" ).second );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	CHECK( constStringSet.insert( "test2" ).second );
	CHECK( constStringSet.size() == 2 );
	CHECK( !constStringSet.empty() );
	// Erase non-existant
	CHECK( constStringSet.erase( "test3" ) == 0 );
	CHECK( constStringSet.size() == 2 );
	CHECK( !constStringSet.empty() );
	// Erase existing
	CHECK( constStringSet.erase( "test" ) == 1 );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	// Erase last item
	CHECK( constStringSet.erase( "test2" ) == 1 );
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
	// Erase last item again
	CHECK( constStringSet.erase( "test2" ) == 0 );
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
}


TEST( StringSet_insert_and_erase_by_iter )
{
	StringSet<> constStringSet;
	// Initial values are correct
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
	// Insert values
	CHECK( constStringSet.insert( "test" ).second );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	CHECK( constStringSet.insert( "test2" ).second );
	CHECK( constStringSet.size() == 2 );
	CHECK( !constStringSet.empty() );
	// Erase test2
	StringSet<>::iterator it = constStringSet.find( "test2" );
	constStringSet.erase( it );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	// Check that the only one left is test1
	it = constStringSet.find( "test" );
	CHECK( it == constStringSet.begin() );
	CHECK( it != constStringSet.end() );
	constStringSet.erase( it );
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
}


TEST( StringSet_insert_and_iter )
{
	StringSet<> constStringSet;
	// Initial values are correct
	CHECK( constStringSet.size() == 0 );
	CHECK( constStringSet.empty() );
	// Insert values
	CHECK( constStringSet.insert( "test" ).second );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	CHECK( constStringSet.insert( "test2" ).second );
	CHECK( constStringSet.size() == 2 );
	CHECK( !constStringSet.empty() );
	// Erase test2
	StringSet<>::iterator it = constStringSet.begin();
	CHECK( it != constStringSet.end() );
	++it;
	CHECK( it != constStringSet.end() );
	++it;
	CHECK( it == constStringSet.end() );
}


TEST( StringSet_const_policy )
{
	typedef StringSet<ConstPolicy> SetType;
	SetType constStringSet;
	char * test = "test";
	char * test2 = "test2";
	
	// Initial values are correct
	std::pair<SetType::iterator, bool> result = 
		constStringSet.insert( test );
	CHECK( result.second );
	CHECK( *result.first == test );

	// Initial values are correct
	result = constStringSet.insert( test2 );
	CHECK( result.second );
	CHECK( *result.first == test2 );
	
	// Erase values
	CHECK( constStringSet.erase( "test" ) );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	
	SetType::iterator it = constStringSet.find( "test2" );
	CHECK( it != constStringSet.end() );
	CHECK( *it == test2 );
}


TEST( StringSet_strdup_policy )
{
	typedef StringSet<StrdupPolicy> SetType;
	SetType constStringSet;
	char * test = "test";
	char * test2 = "test2";
	
	// Initial values are correct
	std::pair<SetType::iterator, bool> result = 
		constStringSet.insert( test );
	CHECK( result.second );
	CHECK( *result.first != test );
	CHECK( strcmp( *result.first, test ) == 0 );

	// Initial values are correct
	result = constStringSet.insert( test2 );
	CHECK( result.second );
	CHECK( *result.first != test2 );
	CHECK( strcmp( *result.first, test2 ) == 0 );
	
	// Erase values
	CHECK( constStringSet.erase( "test" ) );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	
	SetType::iterator it = constStringSet.find( "test2" );
	CHECK( it != constStringSet.end() );
	CHECK( *it != test2 );
	CHECK( strcmp( *it, test2 ) == 0 );
}

TEST( StringSet_free_policy )
{
	typedef StringSet<FreePolicy> SetType;
	SetType constStringSet;
	char * test = (char*)bw_malloc(16);
	strncpy(test, "test", 16);
	char * test2 = (char*)bw_malloc(16);
	strncpy(test2, "test2", 16);
	
	// Initial values are correct
	std::pair<SetType::iterator, bool> result = 
		constStringSet.insert( test );
	CHECK( result.second );
	CHECK( *result.first == test );
	CHECK( strcmp( *result.first, "test" ) == 0 );

	// Initial values are correct
	result = constStringSet.insert( test2 );
	CHECK( result.second );
	CHECK( *result.first == test2 );
	CHECK( strcmp( *result.first, "test2" ) == 0 );
	
	// Erase values
	CHECK( constStringSet.erase( "test" ) );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	
	SetType::iterator it = constStringSet.find( "test2" );
	CHECK( it != constStringSet.end() );
	CHECK( *it == test2 );
	CHECK( strcmp( *it, test2 ) == 0 );
}


TEST( StringSet_delete_policy )
{
	typedef StringSet<FreePolicy> SetType;
	SetType constStringSet;
	char * test = new char[16];
	strncpy(test, "test", 16);
	char * test2 = new char[16];
	strncpy(test2, "test2", 16);
	
	// Initial values are correct
	std::pair<SetType::iterator, bool> result = 
		constStringSet.insert( test );
	CHECK( result.second );
	CHECK( *result.first == test );
	CHECK( strcmp( *result.first, "test" ) == 0 );

	// Initial values are correct
	result = constStringSet.insert( test2 );
	CHECK( result.second );
	CHECK( *result.first == test2 );
	CHECK( strcmp( *result.first, "test2" ) == 0 );
	
	// Erase values
	CHECK( constStringSet.erase( "test" ) );
	CHECK( constStringSet.size() == 1 );
	CHECK( !constStringSet.empty() );
	
	SetType::iterator it = constStringSet.find( "test2" );
	CHECK( it != constStringSet.end() );
	CHECK( *it == test2 );
	CHECK( strcmp( *it, "test2" ) == 0 );
}

BW_END_NAMESPACE

// test_time_queue.cpp
