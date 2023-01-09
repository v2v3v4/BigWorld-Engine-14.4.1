#include "pch.hpp"
#include "blendshapes.hpp"

#include "ExportIterator.h"
#include "utility.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Simple class that disables BlendShapes on creation and enables BlendShapes
 *	on destruction.
 */
class BlendShapesDisabler
{
public:
	BlendShapesDisabler()
	{
		BlendShapes::disable();
	}

	~BlendShapesDisabler()
	{
		BlendShapes::enable();
	}
};


BlendShapes::BlendShapes()
{
	// iterate over all blend shapes
	DependencyExportIterator it( MFn::kBlendShape );

	while( it.isDone() == false )
	{
		// store blend shape
		_blendShapes.push_back( it.item() );

		// get next blend shape
		it.next();
	}
}

BlendShapes::~BlendShapes()
{
	finalise();
	_blendShapes.clear();
}

static uint32 numberOfFrames()
{
	MAnimControl control;
	
	// note the values in MTime correspond to frames if start and end are in MTime::uiUnit() times
	MTime start = control.animationStartTime();
	MTime end = control.animationEndTime();
	
	return (uint32)(end.as( MTime::uiUnit() ) - start.as( MTime::uiUnit() ) + 1);
}

static float getWeight( MFnBlendShapeDeformer& blendShape, uint32 weightIndex, uint32 frame )
{
	MAnimControl control;
	
	// note the values in MTime correspond to frames if start and end are in MTime::uiUnit() times
	MTime start = control.animationStartTime();
	MTime end = control.animationEndTime();
	
	double numberOfFrames = end.as( MTime::uiUnit() ) - start.as( MTime::uiUnit() ) + 1;
	
	if( frame < 0 || frame >= numberOfFrames )
		return 0; // TODO : error?

	// store current time
	MTime time = control.currentTime();
	
	// set the requested frame
	MTime frameTime = start + MTime( frame, MTime::uiUnit() );
	control.setCurrentTime( frameTime );
	
	float weight = blendShape.weight( weightIndex );
	
	// restore old time
	control.setCurrentTime( time );
	
	return weight;
}

bool BlendShapes::hasBlendShape( MObject& object )
{
	// need to check to see if object is a base of one of the blend shapes
	for( uint32 i = 0; i < _blendShapes.size(); ++i )
	{
		// get the blend shape interface for the blend shape i
		MFnBlendShapeDeformer blendShape( _blendShapes[i] );
		
		// get the base objects
		MObjectArray baseObjects;
		
		blendShape.getBaseObjects( baseObjects );

		// check to see if it matches a base object
		for( uint32 j = 0; j < baseObjects.length(); ++j )
			if( object == baseObjects[j] )
				return true;
	}
	
	return false;
}


bool BlendShapes::isBlendShapeTarget( MObject& object )
{
	MFnDependencyNode node( object );
	
	BW::string name = node.name().asChar();

	for( uint32 b = 0; b < count(); ++b )
	{
		if( initialise( b ) == false )
			continue;

		for( uint32 i = 0; i < numBases(); ++i )
			for( uint32 j = 0; j < numTargets( i ); ++j )
			{
				if( target( i, j ).name == name )
				{
					finalise();
					return true;
				}
			}
	}
	
	finalise();
	
	return false;
}

bool BlendShapes::initialise( uint32 index )
{
	finalise();
	
	if( index >= count() )
		return false;
	
	BlendShapesDisabler disabler;
	
	// get the blend shape interface for the blend shape MObject
	MFnBlendShapeDeformer blendShape( _blendShapes[index] );
	
	// get the base objects
	MObjectArray baseObjects;
	
	blendShape.getBaseObjects( baseObjects );
	
	_name = blendShape.name().asChar();
	
	// Build the array of meshes
	MStatus status;
	MDagPathArray dagPathArray;
	DependencyExportIterator it( MFn::kMesh, &status );
	if( status == MStatus::kSuccess )
	{
		for( ; it.isDone() == false; it.next() )
		{
			MFnMesh mesh( it.item(), &status );
			dagPathArray.append( MDagPath::getAPathTo( it.item() ) );
		}
	}

	for( uint32 i = 0; i < baseObjects.length(); ++i )
	{
		MFnDependencyNode base( baseObjects[i] );
		
		_bases.push_back( Object() );
		_bases.back().name = base.name().asChar();
		
		// Find the base object in the mesh array
		uint32 meshIndex = 0;
		bool found = false;
		for ( meshIndex = 0; meshIndex < dagPathArray.length(); ++meshIndex )
		{
			BW::string fullName = dagPathArray[meshIndex].fullPathName().asChar();
			BW::string name = fullName.substr( fullName.find_last_of( "|" ) +  1 );
			if ( name == BW::string(base.name().asChar()) )
			{
				found = true;
				break;
			}
		}

		// If the base object mesh was not found, return as failed
		if (!found)
			return false;

		// Get the base mesh from the mesh array
		MFnMesh mesh( dagPathArray[meshIndex] );

		MPointArray vertexList;
		mesh.getPoints( vertexList, MSpace::kObject );
		
		for( uint32 n = 0; n < vertexList.length(); ++n )
		{
			vertexList[n].cartesianize();
			
			if (ExportSettings::instance().useLegacyOrientation())
			{
				// This legacy code came from the 1.7.2 release
				_bases.back().vertices.push_back(
					vector3<float>(
						-static_cast<float>( vertexList[n].x ),
						static_cast<float>( vertexList[n].y ),
						static_cast<float>( vertexList[n].z ) ) );
			}
			else
			{
				_bases.back().vertices.push_back(
					vector3<float>(
						static_cast<float>( vertexList[n].x ),
						static_cast<float>( vertexList[n].y ),
						-static_cast<float>( vertexList[n].z ) ) );
			}
		}
		
		_targets.push_back( Objects() );

		// get list of weight indices as it can be sparse
		MIntArray weightIndexList;
		blendShape.weightIndexList( weightIndexList );
		
		for( uint32 j = 0; j < blendShape.numWeights(); ++j )
		{
			MObjectArray targetObjects;
	
			blendShape.getTargets( baseObjects[i], weightIndexList[j], targetObjects );
			
			uint32 numTargets = targetObjects.length();
			for( uint32 k = 0; k < numTargets; ++k )
			{
				MFnDependencyNode target( targetObjects[k] );

				_targets[i].push_back( Object() );
				_targets[i][k + j*numTargets].name = target.name().asChar();
				
				// Find the target object in the mesh array
				found = false;
				for ( meshIndex = 0; meshIndex < dagPathArray.length(); ++meshIndex )
				{
					BW::string fullName = dagPathArray[meshIndex].fullPathName().asChar();
					BW::string name = fullName.substr( fullName.find_last_of( "|" ) +  1 );
					if ( name == BW::string(target.name().asChar()) )
					{
						found = true;
						break;
					}
				}

				// If the target object mesh was not found, return as failed
				if (!found)
					return false;

				// Get the base mesh from the mesh array
				MFnMesh mesh( dagPathArray[meshIndex] );
				
				MPointArray vertexList;
				mesh.getPoints( vertexList, MSpace::kObject );
				
				for( uint32 n = 0; n < vertexList.length(); ++n )
				{
					vertexList[n].cartesianize();
					
					if (ExportSettings::instance().useLegacyOrientation())
					{
						// This legacy code came from the 1.7.2 release
						_targets[i][k + j*numTargets].vertices.push_back(
							vector3<float>(
								-static_cast<float>( vertexList[n].x ),
								static_cast<float>( vertexList[n].y ),
								static_cast<float>( vertexList[n].z ) ) );
					}
					else
					{
						_targets[i][k + j*numTargets].vertices.push_back(
							vector3<float>(
								static_cast<float>( vertexList[n].x ),
								static_cast<float>( vertexList[n].y ),
								-static_cast<float>( vertexList[n].z ) ) );
					}

				}
				
				// get the weight for each frame
				for( uint32 n = 0; n < numberOfFrames(); ++n )
				{
					_targets[i][k + j*numTargets].weights.push_back( getWeight( blendShape, weightIndexList[j], n ) );
				}
			}
		}
	}
	
	return true;
}

bool BlendShapes::initialise( MObject& object )
{
	// need to check to see if object is a base of one of the blend shapes
	for( uint32 i = 0; i < _blendShapes.size(); ++i )
	{
		// get the blend shape interface for the blend shape i
		MFnBlendShapeDeformer blendShape( _blendShapes[i] );
		
		// get the base objects
		MObjectArray baseObjects;
		
		blendShape.getBaseObjects( baseObjects );

		// check to see if it matches a base object
		for( uint32 j = 0; j < baseObjects.length(); ++j )
			if( object == baseObjects[j] )
				return initialise( i );
	}
	
	return false;
}

void BlendShapes::finalise()
{
	_name.clear();
	_bases.clear();
	_targets.clear();
}

BW::string BlendShapes::name()
{
	return _name;
}

uint32 BlendShapes::numBases()
{
	return static_cast<uint32>(_bases.size());
}

uint32 BlendShapes::numTargets( uint32 base )
{
	return static_cast<uint32>(_targets[base].size());
}

uint32 BlendShapes::countTargets(  )
{
	size_t count=0;
	for (size_t i=0; i<_targets.size(); i++)
	{
		count += _targets[i].size();
	}
	return static_cast<uint32>(count);
}

BlendShapes::Object& BlendShapes::base( uint32 base )
{
	return _bases[base];
}

BlendShapes::Object& BlendShapes::target( uint32 base, uint32 index )
{
	return _targets[base][index];
}

vector3<float> BlendShapes::delta( uint32 base, uint32 target, uint32 index )
{
	return _targets[base][target].vertices[index] - _bases[base].vertices[index];
}

uint32 BlendShapes::count()
{
	return static_cast<uint32>(_blendShapes.size());
}

void BlendShapes::disable()
{
	MItDependencyNodes it( MFn::kBlendShape );
	
	while( it.isDone() == false )
	{
		MFnBlendShapeDeformer blendShape( it.item() );
		
		// get the envelope attribute plug
		MPlug plug = blendShape.findPlug( "en" );
		
		// set to 0 to disable FFD effect
		plug.setValue( 0.0f );
		
		it.next();
	}
}

void BlendShapes::enable()
{
 	MItDependencyNodes it( MFn::kBlendShape );
	
	while( it.isDone() == false )
	{
		MFnBlendShapeDeformer blendShape( it.item() );
		
		// get the envelope attribute plug
		MPlug plug = blendShape.findPlug("en");
		
		// set to 1 to enable FFD effect
		plug.setValue( 1.0f );
		
		it.next();
	}
}

BW_END_NAMESPACE

