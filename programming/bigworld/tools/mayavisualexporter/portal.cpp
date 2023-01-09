#include "pch.hpp"

#include "portal.hpp"
#include "resmgr/bwresource.hpp"

#include "ExportIterator.h"
#include "utility.hpp"

BW_BEGIN_NAMESPACE

template<typename MObj>
bool isObjectVisible( MObj& obj )
{
	bool visible = true;
	MStatus status;
	MPlug plug = obj.findPlug( "visibility", &status );
	if( status == MS::kSuccess )
		plug.getValue( visible );
	if( visible )
	{
		MPlug plug = obj.findPlug( "v", &status );
		if( status == MS::kSuccess )
			plug.getValue( visible );
	}
	return visible;
}

Portal::Portal()
{
	MStatus status;
	DependencyExportIterator it( MFn::kTransform , &status );

	if( status == MStatus::kSuccess )
	{
		for( ; it.isDone() == false; it.next() )
		{
			MFnDependencyNode fn( it.item() );
			
			// check to see if it is a portal
			MPlug plug = fn.findPlug("Portal", &status);

			if ( status == MS::kSuccess )
			{
				bool active = false;
				plug.getValue(active);

				if ( active && isObjectVisible( fn ) )
				{
					_portals.append( MDagPath::getAPathTo( it.item() ) );
					
				}
			}
		}
	}
}

Portal::~Portal()
{
	finalise();
}

uint32 Portal::count()
{
	return (uint32)_portals.length();
}


bool Portal::initialise( uint32 index )
{
	if( index > _portals.length()-1 )
		return false;
	
	finalise();

	// get child mesh(s?) or just first child
	MStatus status;
	MFnTransform portal(_portals[index], &status);
	if ( status == false )
		return false;

	// get flags
	// Exit flag
	MPlug plug = portal.findPlug("Exit", &status);
	if ( status && _flags == "" )
	{
		// check value
		bool active = false;
		plug.getValue(active);

		if ( active )
			_flags = "invasive";
	}

	// Heaven flag
	plug = portal.findPlug("Heaven", &status);
	if ( status && _flags == "" )
	{
		// check value
		bool active = false;
		plug.getValue(active);

		if ( active )
			_flags = "heaven";
	}

	// Get the label for this portal
	plug = portal.findPlug("Label", &status);
	if ( status )
	{
		MString label;
		plug.getValue(label);

		_label = label.asChar();

	}

	if ( portal.childCount() == 1 )
	{
		//RA: Using the child would ignore the transform...... changing...
		//MObject child = portal.child(0, &status);
		//MObject child = MObject(_portals[index]);
		//MFnTransform portal(, &status);
		//if ( child.hasFn(MFn::kMesh) )
		{
			// get the mesh object
			//MFnMesh mesh(child, &status);
			MFnMesh mesh(_portals[index], &status);

			// store names and points of mesh
			if ( status && isObjectVisible( mesh ) )
			{
				_name =  mesh.fullPathName().asChar();

				// get vertex positions
				MPointArray vertexList;
				mesh.getPoints( vertexList, MSpace::kWorld );
				for( uint32 i = 0; i < vertexList.length(); ++i )
				{
					vertexList[i].cartesianize();

					if (ExportSettings::instance().useLegacyOrientation())
					{
						// This legacy code came from the 1.7.2 release
						_positions.push_back(
							Point3(
								-static_cast<float>( vertexList[i].x ),
								static_cast<float>( vertexList[i].y ),
								static_cast<float>( vertexList[i].z )));
					}
					else
					{
						_positions.push_back(
							Point3(
							    static_cast<float>( vertexList[i].x ),
								static_cast<float>( vertexList[i].y ),
								-static_cast<float>( vertexList[i].z ) ) );
					}
				}
			}
		}
	}

	return true;
}

void Portal::finalise()
{
	_name = "";
	_flags = "";
	_label = "";
	_positions.clear();
}

MDagPathArray& Portal::portals()
{
	return _portals;
}

BW::string Portal::name()
{
	return _name;
}

BW::string Portal::flags()
{
	return _flags;
}

BW::string Portal::label()
{
	return _label;
}

BW::vector<Point3>& Portal::positions()
{
	return _positions;
}

BW_END_NAMESPACE

