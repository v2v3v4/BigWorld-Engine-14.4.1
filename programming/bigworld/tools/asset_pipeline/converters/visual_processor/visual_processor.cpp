#include "visual_processor.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"
#include "moo/visual_common.hpp"
#include "moo/visual.hpp"
#include "moo/visual_manager.hpp"
#include "moo/primitive.hpp"
#include "moo/primitive_manager.hpp"
#include "moo/vertices.hpp"
#include "moo/vertices_manager.hpp"
#include "resmgr/bwresource.hpp"


BW_BEGIN_NAMESPACE

const uint64 VisualProcessor::s_TypeId = Hash64::compute( VisualProcessor::getTypeName() );
const char * VisualProcessor::s_Version = "2.8";

namespace
{
	void loadNodeNames( const DataSectionPtr& nodeSection,
		BW::set<BW::string>& nodeNames )
	{
		BW::string identifier = nodeSection->readString( "identifier" );
		nodeNames.insert( identifier );

		BW::vector< DataSectionPtr > nodeSections;
		nodeSection->openSections( "node", nodeSections );
		BW::vector< DataSectionPtr >::iterator it = nodeSections.begin();
		BW::vector< DataSectionPtr >::iterator end = nodeSections.end();

		while( it != end )
		{
			loadNodeNames( *it++, nodeNames );
		}
	}
}

/* construct a converter with parameters. */
VisualProcessor::VisualProcessor( const BW::string & params ) :
	Converter( params )
{
	BW_GUARD;
}

VisualProcessor::~VisualProcessor()
{
}

/* builds the dependency list for a source file. */
bool VisualProcessor::createDependencies( const BW::string & sourcefile, 
										  const Compiler & compiler,
										  DependencyList & dependencies )
{
	BW_GUARD;

	BW::string baseName =
		sourcefile.substr( 0, sourcefile.find_last_of( '.' ) );
	Moo::VisualLoader< Moo::Visual > loader( baseName, false );

	// open the resource
	DataSectionPtr root = loader.getRootDataSection();
	if (!root)
	{
		ERROR_MSG( "Couldn't open visual %s\n", sourcefile.c_str() );
		return false;
	}

	// Load our primitives
	const BW::string& primitivesResource = loader.getPrimitivesResID();
	BW::string primitivesProcessedResource = primitivesResource + ".processed";

	compiler.resolveRelativePath( primitivesProcessedResource );
	dependencies.addSecondaryOutputFileDependency( 
		primitivesProcessedResource, false );

	return true;
}

/* convert a source file. */
/* work out the output filename & insert it into the converted files vector */
bool VisualProcessor::convert( const BW::string & sourcefile,
							   const Compiler & compiler,
							   BW::vector< BW::string > & intermediateFiles,
							   BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;

	BW::string baseName =
		sourcefile.substr( 0, sourcefile.find_last_of( '.' ) );
	Moo::VisualLoader< Moo::Visual > loader( baseName, false );

	// open the resource
	DataSectionPtr orig = loader.getRootDataSection();
	if (!orig)
	{
		ERROR_MSG( "Couldn't open visual %s\n", sourcefile.c_str() );
		return false;
	}

	BW::string outputFile = baseName + ".visual.processed";
	compiler.resolveOutputPath( outputFile );
	DataSectionPtr root = BWResource::openSection( outputFile, true );
	root->copy( orig, true );

	// Load our primitives
	const BW::string primitivesFileName = loader.getPrimitivesResID() + '/';

	// load the hierarchy
	BW::set<BW::string> nodeNames;
	DataSectionPtr nodeSection = root->openSection( "node" );
	if (nodeSection)
	{
		loadNodeNames( nodeSection, nodeNames );
	}
	else
	{
		nodeNames.insert( "root" );
	}

	// open the renderesets
	BW::vector< DataSectionPtr > renderSets;
	root->openSections( "renderSet", renderSets );

	if (renderSets.size() == 0)
	{
		ERROR_MSG( "Moo::Visual: No rendersets in %s\n", sourcefile.c_str() );
		return false;
	}

	// iterate through the rendersets and create them
	BW::vector< DataSectionPtr >::iterator rsit = renderSets.begin();
	BW::vector< DataSectionPtr >::iterator rsend = renderSets.end();
	while (rsit != rsend)
	{
		DataSectionPtr renderSetSection = *rsit;
		++rsit;
		
		// Read the node identifiers of all nodes that affect this renderset.
		BW::vector< BW::string > nodes;
		renderSetSection->readStrings( "node", nodes );

		// Are there any nodes?
		if (nodes.size())
		{
			// Iterate through the node identifiers.
			BW::vector< BW::string >::iterator it = nodes.begin();
			BW::vector< BW::string >::iterator end = nodes.end();
			while (it != end)
			{
				const BW::string& curNodeName = *it;

				// Find the current node.
				if (nodeNames.find( curNodeName ) == nodeNames.end())
				{
					ERROR_MSG( "Couldn't find node %s in %s\n", 
						curNodeName.c_str(), sourcefile.c_str() );
					return false;
				}

				++it;
			}
		}

		// Open the geometry sections
		BW::vector< DataSectionPtr > geometries;
		renderSetSection->openSections( "geometry", geometries );

		if (geometries.size() == 0)
		{
			ERROR_MSG( "No geometry in renderset in %s\n", sourcefile.c_str() );
			return false;
		}		

		BW::vector< DataSectionPtr >::iterator geit = geometries.begin();
		BW::vector< DataSectionPtr >::iterator geend = geometries.end();

		// iterate through the geometry sections
		while (geit != geend)
		{
			DataSectionPtr geometrySection = *geit;
			++geit;

			// Get a reference to the vertices used by this geometry
			BW::string verticesName = geometrySection->readString( "vertices" );
			if (verticesName.find_first_of( '/' ) == BW::string::npos)
				verticesName = primitivesFileName + verticesName;

			int numNodesValidate = (int)nodes.size();
			// only validate number of nodes if we have to.
			Moo::VerticesPtr vertices = Moo::VerticesManager::instance()->get( verticesName, numNodesValidate );

			if (vertices->nVertices() == 0)
			{
				ERROR_MSG( "No vertex information in \"%s\".\nUnable to load \"%s\".\n",
					verticesName.c_str(), sourcefile.c_str() );
				return false;
			}

			// Get a reference to the indices used by this geometry
			BW::string indicesName = geometrySection->readString( "primitive" );
			if (indicesName.find_first_of( '/' ) == BW::string::npos)
				indicesName = primitivesFileName + indicesName;
			Moo::PrimitivePtr primitives = Moo::PrimitiveManager::instance()->get( indicesName );
			if (primitives->nIndices() == 0)
			{
				ERROR_MSG( "No indices information in \"%s\".\nUnable to load \"%s\".\n",
					indicesName.c_str(), sourcefile.c_str() );
				return false;
			}

			// Open the primitive group descriptions
			BW::vector< DataSectionPtr > primitiveGroups;
			geometrySection->openSections( "primitiveGroup", primitiveGroups );

			BW::vector< DataSectionPtr >::iterator prit = primitiveGroups.begin();
			BW::vector< DataSectionPtr >::iterator prend = primitiveGroups.end();

			// Precalculate the group origins
			primitives->calcGroupOrigins( vertices );
			
			// Iterate through the primitive group descriptions
			while (prit != prend)
			{
				DataSectionPtr primitiveGroupSection = *prit;
				++prit;

				uint32 groupIndex = primitiveGroupSection->asInt();

				// Inject the primitive origin
				primitiveGroupSection->writeVector3( "groupOrigin", 
					primitives->origin( groupIndex ) );
			}
		}
	}

	// Now output the .visual file
	root->save( outputFile );
	outputFiles.push_back( outputFile );

	return true;
}
BW_END_NAMESPACE
