#include "pch.hpp"

#include "skin.hpp"

#include "ExportIterator.h"
#include "utility.hpp"

BW_BEGIN_NAMESPACE

BW::vector<BoneVertex> Skin::_empty( 0 );


Skin::Skin() :
	_bones( 0 )
{
	MStatus status;
	DependencyExportIterator it( MFn::kSkinClusterFilter, &status );

	if( status == MStatus::kSuccess )
	{
		for( ; it.isDone() == false; it.next() )
		{
			_skins.push_back( it.item() );
		}
	}
}

Skin::~Skin()
{
	finalise();
}

uint32 Skin::count()
{
	return (uint32)_skins.size();
}

bool Skin::initialise( uint32 index )
{
	if( index > _skins.size()-1 )
		return false;
	
	// set to first frame of the animation
	MAnimControl control;
	
	// store current time
	MTime time = control.currentTime();
	
	control.setCurrentTime( control.animationStartTime() );
	
	// determine bone indexes
	
	// clear data in case a skin is already initialised
	finalise();
	
	// get the skin cluster from the MObject
	MFnSkinCluster skin( _skins[index] );
		
	_name = skin.name().asChar();

	MStatus status;
	MDagPathArray influences;

	// get dag paths for objects influencing the skin
	skin.influenceObjects( influences, &status );
	
	if( status == MStatus::kFailure )
		return false;
	
	// BW::map the dag paths to indexes
	for( uint32 i = 0; i < influences.length(); ++i )
	{
		_boneIndexes[ influences[i].fullPathName().asChar() ] = _bones;
		_bonePaths[ _bones ] = influences[i];
		++_bones;
	}
	
	// get the transforms for each bone influencing the skin
	_transforms.resize( _bones );
	_relativeTransforms.resize( _bones );
	
	// for each bone
	for( uint32 i = 0; i < _bones; ++i )
	{
		MFloatMatrix matrix( _bonePaths[i].inclusiveMatrix().matrix );

		if (ExportSettings::instance().useLegacyOrientation())
		{
			// This legacy code came from the 1.7.2 release
			matrix[0][0] *= -1;
			matrix[0][1] *= -1;
			matrix[0][2] *= -1;
		}
		else
		{
			matrix[2][0] *= -1;
			matrix[2][1] *= -1;
			matrix[2][2] *= -1;
		}

		memcpy( &_transforms[i], matrix.matrix, sizeof(float) * 4 * 4 );
		
		MFloatMatrix parent( _bonePaths[i].exclusiveMatrix().matrix );
		
		if (ExportSettings::instance().useLegacyOrientation())
		{
			// This legacy code came from the 1.7.2 release
			parent[0][0] *= -1;
			parent[0][1] *= -1;
			parent[0][2] *= -1;
		}
		else
		{
			parent[2][0] *= -1;
			parent[2][1] *= -1;
			parent[2][2] *= -1;
		}

		MFloatMatrix relative =  matrix * parent.inverse();
		memcpy( &_relativeTransforms[i], relative.matrix, sizeof(float) * 4 * 4 );
	}
	
	// gather vertex data for influenced geometry
	uint32 numberOfGeometry = skin.numOutputConnections();
	
	for( uint32 i = 0; i < numberOfGeometry; ++i )
	{
		uint32 index = skin.indexForOutputConnection( i );
		
		MDagPath geometry;
		skin.getPathAtIndex( index, geometry );
		
		_meshes.append( geometry );
		_boneSets.push_back( BoneSet() );
		
		BW::vector<BoneVertex>& vertices = _vertices[ geometry.fullPathName().asChar() ];
		
		// get position for vertices
		if( geometry.hasFn( MFn::kMesh ) == false )
			return false;
		
		MFnMesh mesh( geometry );
		
		MPointArray vertexList;
		mesh.getPoints( vertexList, MSpace::kWorld );
		
		for( uint32 h = 0; h < vertexList.length(); ++h )
		{
			vertexList[h].cartesianize();
			
			if (ExportSettings::instance().useLegacyOrientation())
			{
				// This legacy code came from the 1.7.2 release
				vertices.push_back(
					BoneVertex(
						Point3(
							-static_cast<float>( vertexList[h].x ),
							static_cast<float>( vertexList[h].y ),
							static_cast<float>( vertexList[h].z ) ) ) );
			}
			else
			{
				vertices.push_back(
					BoneVertex(
						Point3(
							static_cast<float>( vertexList[h].x ),
							static_cast<float>( vertexList[h].y ),
							-static_cast<float>( vertexList[h].z ) ) ) );
			}
		}

		MItGeometry it( geometry );
		
		uint32 j = 0;
		
		for( ; it.isDone() == false; it.next() )
		{
			MObject component = it.component();
			
			if( j > vertices.size()-1 )
				return false;
			
			// get the 3 highest weights from the skin
			MFloatArray weights;
			
			unsigned int count = influences.length();
			skin.getWeights( geometry, component, weights, count );
			
			
			for( uint32 k = 0; k < count; ++k )
			{
				vertices[j].addWeight( k, weights[k] );
			}
			
			vertices[j].normaliseWeights();

			// NOTE for some reason sorting the ( index1, weight1 ), ( index2, weight2 ) and ( index3, weight3 ) royally screws the mesh up?? - even though they are sorted properly
			vertices[j].sortWeights();
			
			if( vertices[j].weight1 != 0 )
			{
				// add the bone to the set if needed
				_boneSets[i].addBone( _bonePaths[vertices[j].index1].fullPathName().asChar() );
				// fix the index
				if( _boneSets[i].indexes.find( _bonePaths[vertices[j].index1].fullPathName().asChar() ) == _boneSets[i].indexes.end() )
				{
					printf( "Bone not found in Bone Set: %s\n:", _bonePaths[vertices[j].index1].fullPathName().asChar() );
					fflush( stdout );
				}
				vertices[j].index1 = _boneSets[i].indexes[_bonePaths[vertices[j].index1].fullPathName().asChar()];
			}

			if( vertices[j].weight2 != 0 )
			{
				// add the bone to the set if needed
				_boneSets[i].addBone( _bonePaths[vertices[j].index2].fullPathName().asChar() );
				// fix the index
				if( _boneSets[i].indexes.find( _bonePaths[vertices[j].index2].fullPathName().asChar() ) == _boneSets[i].indexes.end() )
				{
					printf( "Bone not found in Bone Set: %s\n:", _bonePaths[vertices[j].index2].fullPathName().asChar() );
					fflush( stdout );
				}
				vertices[j].index2 = _boneSets[i].indexes[_bonePaths[vertices[j].index2].fullPathName().asChar()];
			}
			
			if( vertices[j].weight3 != 0 )
			{
				// add the bone to the set if needed
				_boneSets[i].addBone( _bonePaths[vertices[j].index3].fullPathName().asChar() );
				// fix the index
				if( _boneSets[i].indexes.find( _bonePaths[vertices[j].index3].fullPathName().asChar() ) == _boneSets[i].indexes.end() )
				{
					printf( "Bone not found in Bone Set: %s\n:", _bonePaths[vertices[j].index3].fullPathName().asChar() );
					fflush( stdout );
				}
				vertices[j].index3 = _boneSets[i].indexes[_bonePaths[vertices[j].index3].fullPathName().asChar()];
			}
			++j;
		}
		
		if( j < vertices.size() )
			return false;
	}
	
	// restore initial time
	control.setCurrentTime( time );
	
	return true;
}

void Skin::trim( MDagPathArray& meshes )
{
	uint32 i = 0;
	while (i < _skins.size())
	{
		MFnSkinCluster skin( _skins[i] );
		// gather vertex data for influenced geometry
		uint32 numberOfGeometry = skin.numOutputConnections();

		bool used = false;
		
		for( uint32 j = 0; j < numberOfGeometry; ++j )
		{
			uint32 index = skin.indexForOutputConnection( j );
			
			MDagPath geometry;
			skin.getPathAtIndex( index, geometry );
			for (uint32 k = 0; k < meshes.length(); k++)
			{
				if (meshes[k] == geometry)
					used = true;
			}
		}
		if (used)
			i++;
		else
			_skins.erase( _skins.begin() + i );
	}
}

void Skin::finalise()
{
	_boneSets.clear();
	_meshes.clear();
	_vertices.clear();
	_boneIndexes.clear();
	_bonePaths.clear();
	_transforms.clear();
	_relativeTransforms.clear();
	_bones = 0;
}

BW::string Skin::name()
{
	return _name;
}

BW::vector<MObject>& Skin::skins()
{
	return _skins;
}


MDagPathArray& Skin::meshes()
{
	return _meshes;
}

uint32 Skin::numberOfBones() 
{
	return _bones;
}

uint32 Skin::numberOfVertices( const BW::string& path )
{
	BW::map< BW::string, BW::vector<BoneVertex> >::iterator it = _vertices.find( path );
	if( it == _vertices.end() )
		return 0;
	else
		return (uint32)it->second.size();
}

BW::vector<BoneVertex>& Skin::vertices( const BW::string& path ) 
{
	BW::map< BW::string, BW::vector<BoneVertex> >::iterator it = _vertices.find( path );
	if( it == _vertices.end() )
		return _empty;
	else
		return it->second;
}

BW::vector<BW::string> Skin::paths()
{
	BW::vector<BW::string> objects;
	
	for( BW::map< BW::string, BW::vector<BoneVertex> >::iterator it = _vertices.begin(); it != _vertices.end(); ++it )
		objects.push_back( it->first );

	return objects;
}

BW::string Skin::boneNameFull( uint32 bone ) 
{
	return _bonePaths[bone].fullPathName().asChar();
}

BW::string Skin::boneNamePartial( uint32 bone ) 
{
	return _bonePaths[bone].partialPathName().asChar();
}

MDagPath Skin::boneDAG( uint32 bone )
{
	return _bonePaths[bone];
}

BoneSet& Skin::boneSet( BW::string mesh )
{
	for( uint32 i = 0; i < _meshes.length(); ++i )
		if( mesh == _meshes[i].fullPathName().asChar() )
			return _boneSets[i];
		
	// TODO : error?
	return _boneSets[0];
}

matrix4<float> Skin::transform( uint32 bone, bool relative ) 
{
	if( relative )
		return _relativeTransforms[bone];
	else
		return _transforms[bone];
}

matrix4<float> Skin::transform( const BW::string& boneName, bool relative ) 
{
	BW::map<BW::string, uint32>::iterator it = _boneIndexes.find( boneName );
	
	if( it == _boneIndexes.end() )
		return matrix4<float>();
	else
		return transform( it->second, relative );
}

BW_END_NAMESPACE

