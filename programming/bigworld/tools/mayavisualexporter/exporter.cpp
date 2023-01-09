#include "stdafx.hpp"

#include "exporter.hpp"
#include "skin.hpp"
#include "matrix.hpp"
#include "heirarchy.hpp"
#include "mesh.hpp"

MStatus ea::doIt( const MArgList& args )
{
	//~ Skin skin;
	
	//~ printf( "Skins found: %i\n", skin.count() );
	
	//~ skin.initialise( 0 );
	
	//~ printf( "Skin vertices: %i\n", skin.vertices( skin.meshes()[0].fullPathName().asChar() ).size() );
	
	//~ printf( "Number of bones: %i\n", skin.numberOfBones() );
	
	//~ for( uint32 i = 0; i < skin.meshes().length(); ++i )
		//~ printf( "Mesh %i: %s\n", i, skin.meshes()[i].fullPathName().asChar() );
	
	//~ Heirarchy heirarchy;
	
	//~ heirarchy.getSkeleton( skin );
	
	//~ for( uint32 i = 0; i < heirarchy.numberOfFrames(); ++i )
	//~ {
		//~ printf( "Name: %s, Frame: %i\n", heirarchy.name().c_str(), i );
		//~ heirarchy.transform( i, false ).dump();
		//~ printf( "-------------------------------------------------\n" );
		//~ heirarchy.transform( i, true ).dump();
	//~ }
	
	Mesh mesh;
	
	printf( "Meshes found: %i\n", mesh.count() );
	
	for( uint32 i = 0; i < mesh.count(); ++i )
	{
		printf( "Mesh %2i: %s\n", i, mesh.meshes()[i].fullPathName().asChar() );
	}
	
	mesh.initialise( 0 );
	
	//~ printf( "Positions ====\n" );
	//~ for( uint32 i = 0; i < mesh.positions().size(); ++i )
	//~ {
		//~ printf( "( %6.2f, %6.2f, %6.2f )\n", mesh.positions()[i].x, mesh.positions()[i].y, mesh.positions()[i].z );
	//~ }
	
	//~ printf( "Normals ======\n" );
	//~ for( uint32 i = 0; i < mesh.normals().size(); ++i )
	//~ {
		//~ printf( "( %6.2f, %6.2f, %6.2f )\n", mesh.normals()[i].x, mesh.normals()[i].y, mesh.normals()[i].z );
	//~ }
	
	//~ printf( "UVs ==========\n" );
	//~ for( uint32 i = 0; i < mesh.uvs().size(); ++i )
	//~ {
		//~ printf( "( %6.2f, %6.2f )\n", mesh.uvs()[i].u, mesh.uvs()[i].v );
	//~ }
	
	printf( "Faces: %i =========\n", mesh.faces().size() );
	for( uint32 i = 0; i < mesh.faces().size(); ++i )
		printf( "%2i %2i %2i | %2i %2i %2i | %2i %2i %2i | %i\n", mesh.faces()[i].positionIndex[0], mesh.faces()[i].positionIndex[1], mesh.faces()[i].positionIndex[2],
											   mesh.faces()[i].normalIndex[0], mesh.faces()[i].normalIndex[1], mesh.faces()[i].normalIndex[2],
											   mesh.faces()[i].uvIndex[0], mesh.faces()[i].uvIndex[1], mesh.faces()[i].uvIndex[2], mesh.faces()[i].materialIndex );

	printf( "Materials: %i =======\n", mesh.materials().size() );
	for( uint32 i = 0; i < mesh.materials().size(); ++i )
		printf( "%s: %s\n", mesh.materials()[i].name.c_str(), mesh.materials()[i].texture.c_str() );
	
	fflush( stdout );

    return MStatus::kSuccess;
}