#include "pch.hpp"

#include <maya/MStringArray.h>
#include <maya/MItDag.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>

#include "VisualFileTranslator.hpp"
#include "expsets.hpp"
#include "mesh.hpp"
#include "portal.hpp"
#include "blendshapes.hpp"
#include "visual_mesh.hpp"
#include "visual_envelope.hpp"
#include "visual_portal.hpp"
#include "hull_mesh.hpp"
#include "bsp_generator.hpp"
#include "hierarchy.hpp"
#include "constraints.hpp"
#include "utility.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/dprintf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/binaryfile.hpp"
#include "cstdmf/string_builder.hpp"
#include "cstdmf/log_msg.hpp"
#include "resmgr/primitive_file.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/xml_section.hpp"
#include "gizmo/meta_data_creation_utility.hpp"
#include "math/blend_transform.hpp"
#include "../resourcechecker/visual_checker.hpp"
#include "exportiterator.h"
#include "node_catalogue_holder.hpp"
#include "data_section_cache_purger.hpp"
#include "moo/node.hpp"
#include "resource.h"
#include <commctrl.h>
#include <CommDlg.h>
#include "exporter_dialog.hpp"

#include "maya/MFileIO.h"


// Defines
#define DEBUG_PRIMITIVE_FILE 0
#define DEBUG_ANIMATION_FILE 0
#define DEBUG_VISUAL_FILE 0


namespace
{
	class MELErrorMessages
	{
		BW::string errorString;
		// Stop the errorString from growing out of hand outside of the scope of
		// a reporting mechanism (like the ScopedMELErrorVariableHandler)
		bool isInScope;

	public:
		MELErrorMessages()
			: isInScope(false)
		{ }

		void clear() { errorString.clear(); }

		const BW::string& str() const { return errorString; }

		void setIsInScope(bool val) { isInScope = val; }

		void addString(const BW::string& errStr)
		{
			if ( !isInScope )
				return;

			if ( ! errorString.empty() )
				errorString += "; ";
			errorString += errStr;
		}

		void addMultiLineString(const BW::string& errStr)
		{
			if ( !isInScope )
				return;

			size_t start_idx = 0;
			size_t idx;
			while ( (idx = errStr.find('\n', start_idx)) != BW::string::npos )
			{
				addString(errStr.substr(start_idx, (idx - start_idx)));
				start_idx = idx+1;
			}
			if ( start_idx < errStr.size() )
				addString(errStr.substr(start_idx));
		}
	};

	// This is all handled in a single thread, so global/static access should be ok.
	// I didn't think that it needed to be a VisualFileTranslator member
	MELErrorMessages melErrorMessages;

	struct ScopedMELErrorVariableHandler
	{
		ScopedMELErrorVariableHandler()
		{
			MGlobal::executeCommand("global string $gBWVisualFileError = \"\"");
			melErrorMessages.clear();
			melErrorMessages.setIsInScope(true);
		}

		~ScopedMELErrorVariableHandler()
		{
			BW::string mayaAssignErrorCmd = BW::string("$gBWVisualFileError = \"") + melErrorMessages.str() + "\"";
			MGlobal::executeCommand(mayaAssignErrorCmd.c_str());
			melErrorMessages.setIsInScope(false);
		}
	};

	class OriginTransformer
	{
	public:
		OriginTransformer( MDagPathArray & rootTransforms )
			: rootTransforms_( rootTransforms )
		{
			// Calculate the origin offset by taking the average position of 
			// all the root transforms rotate pivots
			for ( uint32 i = 0; i < rootTransforms_.length(); ++i )
			{
				MFnTransform rootTransform( rootTransforms_[i] );
				originOffset_ += rootTransform.rotatePivot( MSpace::kWorld ) / 
					rootTransforms_.length();
			}

			// Apply the origin offset to all root transforms
			for ( uint32 i = 0; i < rootTransforms_.length(); ++i )
			{
				MFnTransform rootTransform( rootTransforms_[i] );
				for ( uint32 j = 0; j < rootTransform.attributeCount(); ++j)
				{
					MFnAttribute attribute( rootTransform.attribute( j ) );
					MPlug plug( rootTransform.object(), attribute.object() );
					if (plug.isLocked())
					{
						plug.setLocked( false );
						lockedPlugs_.append( plug );
					}
				}
				rootTransform.translateBy( -originOffset_, MSpace::kWorld );
			}
		}

		~OriginTransformer()
		{
			// Undo the origin offset to all root transforms
			for ( uint32 i = 0; i < rootTransforms_.length(); ++i )
			{
				MFnTransform rootTransform( rootTransforms_[i] );
				rootTransform.translateBy( originOffset_, MSpace::kWorld );
			}

			for ( uint32 i = 0; i < lockedPlugs_.length(); ++i )
			{
				lockedPlugs_[i].setLocked( true );
			}
		}

		MDagPathArray rootTransforms_;
		MPlugArray lockedPlugs_;
		MVector originOffset_;
	};
}

BW_BEGIN_NAMESPACE

MDagPath findDagPath( BW::string path );

void addAllHardpoints(Hierarchy& hierarchy)
{
	MStatus status;
	ExportIterator it( MFn::kTransform, &status );

	if( status == MStatus::kSuccess )
	{
		for( ; it.isDone() == false; it.next() )
		{
			MFnTransform transform( it.item() );

			BW::string path = transform.fullPathName().asChar();
			BW::string checkPath = path;
			if (checkPath.find_last_of("|") != BW::string::npos)
			{
				checkPath = checkPath.substr(checkPath.find_last_of("|"));
			}
			if (checkPath.find("HP_") != BW::string::npos)
				hierarchy.addNode( path, findDagPath( path ) );
		}
	}
}


void addAllLocators(Hierarchy& hierarchy)
{
	MStatus status;
	ExportIterator it( MFn::kLocator, &status );

	if (status == MStatus::kSuccess)
	{
		for( ; it.isDone() == false; it.next() )
		{
			MFnDagNode locatorNode( it.item() );

			for (uint i = 0; i < locatorNode.parentCount(); ++i)
			{
				MFnDagNode transform( locatorNode.parent( i ) );
				BW::string path = transform.fullPathName().asChar();
				hierarchy.addNode( path, findDagPath( path ) );
			}
		}
	}
}

bool checkAnyRowHasZeroScale( const Vector3& row0, const Vector3& row1, 
							 const Vector3& row2 )
{
	// returns true if the given rotation/scale matrix contains
	// a row of zero length. If we try and make a quaternion
	// from such a matrix, with the code we have for this purpose,
	// you an end up with NaN in the result.
	const float length0 = row0.length();
	const float length1 = row1.length();
	const float length2 = row2.length();

	bool retVal = false;
	retVal |= almostZero( length0 );
	retVal |= almostZero( length1 );
	retVal |= almostZero( length2 );

	return retVal;
}

void exportTree( DataSectionPtr pParentSection, Hierarchy& hierarchy, 
						BW::string parent = "", bool* hasInvalidTransforms = 0 )
{
// RA: Why not export hard point children??
//	if( parent.find( "HP_" ) != BW::string::npos )
//		return;
	DataSectionPtr pThisSection = pParentSection;

	pThisSection = pParentSection->newSection( "node" );

	if (pThisSection )
	{
		if (parent == "")
		{
			pThisSection->writeString( "identifier", "Scene Root" );
		}
		else
		{
			const BW::string hierarchyName = hierarchy.name();
			const size_t separaterPos = hierarchyName.find_last_of( "|" );
			const BW::string idName = hierarchyName.substr( separaterPos + 1 );
			pThisSection->writeString( "identifier",  idName );
		}

		const matrix4<float> m = hierarchy.transform( 0, true );

		const Vector3 row0( m.v11, m.v21, m.v31 );
		const Vector3 row1( m.v12, m.v22, m.v32 );
		const Vector3 row2( m.v13, m.v23, m.v33 );
		Vector3 row3( m.v14, m.v24, m.v34 );

		row3 *= ExportSettings::instance().unitScale();
		pThisSection->writeVector3( "transform/row0", row0 );
		pThisSection->writeVector3( "transform/row1", row1 );
		pThisSection->writeVector3( "transform/row2", row2 );
		pThisSection->writeVector3( "transform/row3", row3 );

		// Check if the node transformation is valid. If this fails we should
		// trigger a warning to be displayed but not actually stop the export.
		// The warning is not displayed by this function, the caller is
		// responsible for this.

		// Only do the check if we want to know about failure and haven't
		// already failed.
		if (hasInvalidTransforms && ((*hasInvalidTransforms) == false))
		{
			if (!checkAnyRowHasZeroScale( row0, row1, row2 ))
			{
				// If the rotation matrix contains zero scaling in a row
				// the quaternion length test below will not work because NaN 
				// can appear in the created quaternion
				Matrix testMatrix = pThisSection->readMatrix34( "transform", 
					Matrix::identity );

				BlendTransform BTTest = BlendTransform( testMatrix );
				Quaternion quat = BTTest.rotation();
				if (!almostEqual( quat.length(), 1.0f ))
				{
					//::MessageBox(0, L"invalid quat", L"", MB_ICONERROR);
					*hasInvalidTransforms = true;
				}
			}
			else
			{
				//::MessageBox(0, L"scale contains zero", L"", MB_ICONERROR);
				*hasInvalidTransforms = true;
			}
		}
	}

	for (int i = 0; i < (int)hierarchy.children().size(); i++)
	{
		exportTree( pThisSection, hierarchy.child( hierarchy.children()[i] ), 
			parent + "|" + hierarchy.name(), hasInvalidTransforms );
	}
}


void exportAnimationXmlHeader(
	const BW::string& animDebugXmlFileName, const BW::string& animName, float numberOfFrames, int nChannels )
{
	DataSectionPtr spFileRoot = BWResource::openSection( animDebugXmlFileName );
	if ( !spFileRoot )
		spFileRoot = new XMLSection( "AnimationXml" );

	DataSectionPtr spFile = spFileRoot->newSection( animName );

	// Print the animation header
	DataSectionPtr pAH = spFile->newSection( "AnimHeader" );
	pAH->writeString( "AnimName", animName );
	pAH->writeFloat( "NumFrame", numberOfFrames );
	pAH->writeInt( "NumChannels", nChannels );

	spFileRoot->save( animDebugXmlFileName );
}

void exportAnimationXml(
	DataSectionPtr spParentNode, Hierarchy& hierarchy, bool stripRefPrefix = false )
{
	DataSectionPtr pNode = spParentNode->newSection( "Node" );

	if( hierarchy.name() != "" )
	{
		if (stripRefPrefix)
			hierarchy.name( stripReferencePrefix( hierarchy.name() ) );
		
		typedef std::pair<float, Vector3> ScaleKey;
		typedef std::pair<float, Vector3> PositionKey;
		typedef std::pair<float, Quaternion> RotationKey;

		BW::vector< int > boundTable;
		BW::vector< ScaleKey > scales;
		BW::vector< PositionKey > positions;
		BW::vector< RotationKey > rotations;

		int firstFrame = 0;
		int lastFrame = hierarchy.numberOfFrames() - 1;

		float time = 0;
		for (int i = firstFrame; i <= lastFrame; i++)
		{
			matrix4<float> theMatrix = hierarchy.transform( i, true );

			Matrix m;
			m.setIdentity();
			m[0][0] = theMatrix.v11;
			m[0][1] = theMatrix.v21;
			m[0][2] = theMatrix.v31;
			m[0][3] = theMatrix.v41;
			m[1][0] = theMatrix.v12;
			m[1][1] = theMatrix.v22;
			m[1][2] = theMatrix.v32;
			m[1][3] = theMatrix.v42;
			m[2][0] = theMatrix.v13;
			m[2][1] = theMatrix.v23;
			m[2][2] = theMatrix.v33;
			m[2][3] = theMatrix.v43;
			m[3][0] = theMatrix.v14;
			m[3][1] = theMatrix.v24;
			m[3][2] = theMatrix.v34;
			m[3][3] = theMatrix.v44;
			BlendTransform bt( m );

			// Normalise the rotation quaternion
			bt.normaliseRotation();

			boundTable.push_back( i - firstFrame + 1 );
			scales.push_back( ScaleKey( time, bt.scaling() ) );
			positions.push_back( PositionKey( time, bt.translation() * ExportSettings::instance().unitScale() ) );
			rotations.push_back( RotationKey( time, bt.rotation() ) );
			time = time + 1;
		}

		BW::string nodeName =
			(stripRefPrefix) ?
				( stripReferencePrefix( hierarchy.path().substr( hierarchy.path().find_last_of( "|" ) + 1 ) ) )
				:
				( hierarchy.path().substr( hierarchy.path().find_last_of( "|" ) + 1 ) );

		pNode->writeString( "NodeName", nodeName );

		DataSectionPtr pScales = pNode->newSection( "Scales" );
		BW::vector< ScaleKey >::iterator scaleIt;
		for (scaleIt = scales.begin(); scaleIt != scales.end(); ++scaleIt)
		{
			DataSectionPtr pScale = pScales->newSection( "Scale" );
			pScale->writeFloat( "Key", scaleIt->first );
			pScale->writeVector3( "Value", scaleIt->second ); 
		}

		DataSectionPtr pPositions = pNode->newSection( "Positions" );
		BW::vector< PositionKey >::iterator posIt;
		for (posIt = positions.begin(); posIt != positions.end(); ++posIt)
		{
			DataSectionPtr pPosition = pPositions->newSection( "Position" );
			pPosition->writeFloat( "Key", posIt->first );
			pPosition->writeVector3( "Value", posIt->second );
		}

		DataSectionPtr pRotations = pNode->newSection( "Rotations" );
		BW::vector< RotationKey >::iterator rotIt;
		for (rotIt = rotations.begin(); rotIt != rotations.end(); ++rotIt)
		{
			DataSectionPtr pRotation = pRotations->newSection( "Rotation" );
			pRotation->writeFloat( "Key", rotIt->first );
			pRotation->writeVector4( "Value", Vector4(rotIt->second.x, rotIt->second.y, rotIt->second.z, rotIt->second.w) );
		}

		DataSectionPtr pBTS = pNode->newSection( "BoundTableScales" );
		pBTS->writeInts( "Value", boundTable );

		DataSectionPtr pBTP = pNode->newSection( "BoundTablePositions" );
		pBTP->writeInts( "Value", boundTable );

		DataSectionPtr pBTR = pNode->newSection( "BoundTableRotations" );
		pBTR->writeInts( "Value", boundTable );
	}

	BW::vector<BW::string> children = hierarchy.children();
	for (uint i = 0; i < children.size(); i++)
	{
		exportAnimationXml( pNode, hierarchy.child( children[i] ), stripRefPrefix );
	}
}

void exportAnimation( BinaryFile& animFile, Hierarchy& hierarchy, bool stripRefPrefix = false )
{
	if( hierarchy.name() != "" )
	{
		if (stripRefPrefix)
			hierarchy.name( stripReferencePrefix( hierarchy.name() ) );
		
		typedef std::pair<float, Vector3> ScaleKey;
		typedef std::pair<float, Vector3> PositionKey;
		typedef std::pair<float, Quaternion> RotationKey;

		BW::vector< int > boundTable;
		BW::vector< ScaleKey > scales;
		BW::vector< PositionKey > positions;
		BW::vector< RotationKey > rotations;

			int firstFrame = 0;//~ExportSettings::instance().firstFrame();
			int lastFrame = hierarchy.numberOfFrames() - 1;//~ExportSettings::instance().lastFrame();

			float time = 0;
			for (int i = firstFrame; i <= lastFrame; i++)
			{
				matrix4<float> theMatrix = hierarchy.transform( i, true );

				Matrix m;
				m.setIdentity();
				m[0][0] = theMatrix.v11;
				m[0][1] = theMatrix.v21;
				m[0][2] = theMatrix.v31;
				m[0][3] = theMatrix.v41;
				m[1][0] = theMatrix.v12;
				m[1][1] = theMatrix.v22;
				m[1][2] = theMatrix.v32;
				m[1][3] = theMatrix.v42;
				m[2][0] = theMatrix.v13;
				m[2][1] = theMatrix.v23;
				m[2][2] = theMatrix.v33;
				m[2][3] = theMatrix.v43;
				m[3][0] = theMatrix.v14;
				m[3][1] = theMatrix.v24;
				m[3][2] = theMatrix.v34;
				m[3][3] = theMatrix.v44;
				BlendTransform bt( m );

				// Normalise the rotation quaternion
				bt.normaliseRotation();

				boundTable.push_back( i - firstFrame + 1 );
				scales.push_back( ScaleKey( time, bt.scaling() ) );
				positions.push_back( PositionKey( time, bt.translation() * ExportSettings::instance().unitScale() ) );
				rotations.push_back( RotationKey( time, bt.rotation() ) );
				time = time + 1;
			}

			{
				animFile << int(1);

				if (stripRefPrefix)
					animFile << stripReferencePrefix( hierarchy.path().substr( hierarchy.path().find_last_of( "|" ) + 1 ) );
				else
					animFile << hierarchy.path().substr( hierarchy.path().find_last_of( "|" ) + 1 );
			}

			animFile.writeSequence( scales );
			animFile.writeSequence( positions );
			animFile.writeSequence( rotations );
			animFile.writeSequence( boundTable );
			animFile.writeSequence( boundTable );
			animFile.writeSequence( boundTable );
	}

	BW::vector<BW::string> children = hierarchy.children( true );

	for (uint i = 0; i < children.size(); i++)
	{
		exportAnimation( animFile, hierarchy.child( children[i] ), stripRefPrefix );
	}
}

bool isShell( const BW::string& resName )
{
	BW::string s = resName;
	std::replace( s.begin(), s.end(), '\\', '/' );
	return ( s.size() > 7 && s.substr( 0, 7 ) == "shells/" ) || (s.find( "/shells/" ) != BW::string::npos);
}


/**
 *	Export the user-defined hull for the shell.
 *	Convert all triangles in the hull model to planes,
 *	and detect if any planes are portals.
 */
void exportHull( DataSectionPtr pVisualSection, BW::vector<HullMeshPtr>& hullMeshes )
{
	//Accumulate triangles into an accumulator HullMesh.
	//The Hull Mesh stores plane equations, and works out
	//which of the planes are also portals.
	HullMeshPtr hullMesh = new HullMesh();
	while ( hullMeshes.size() )
	{
		hullMeshes.back()->add( *hullMesh );
		hullMeshes.pop_back();
	}

	//Save the boundary planes, and any attached portals.
	if ( !(hullMesh->exportHull( pVisualSection )) )
	{
		melErrorMessages.addMultiLineString(hullMesh->errorText());
	}
}


/**
 *	Generate a hull for a shell.
 *  Use the 6 planes of the bounding box of the visual,
 *	and make sure the portals are on a plane.
 */
void generateHull( DataSectionPtr pVisualSection, const BoundingBox& bb )
{
	Vector3 bbMin( bb.minBounds() );
	Vector3 bbMax( bb.maxBounds() );

	// now write out the boundary
	for (int b = 0; b < 6; b++)
	{
		DataSectionPtr pBoundary = pVisualSection->newSection( "boundary" );

		// figure out the boundary plane in world coordinates
		int sign = 1 - (b&1)*2;
		int axis = b>>1;
		Vector3 normal(
			sign * ((axis==0)?1.f:0.f),
			sign * ((axis==1)?1.f:0.f),
			sign * ((axis==2)?1.f:0.f) );
		float d = sign > 0 ? bbMin[axis] : -bbMax[axis];

		Vector3 localCentre = normal * d;
		Vector3 localNormal = normal;
		localNormal.normalise();
		PlaneEq localPlane( localNormal, localNormal.dotProduct( localCentre ) );

		pBoundary->writeVector3( "normal", localPlane.normal() );
		pBoundary->writeFloat( "d", localPlane.d() );
	}
}

/**
 *	This method reads a boundary section, and interprets it as
 *	a plane equation.  The plane equation is returned via the
 *	ret parameter.
 */
void planeFromBoundarySection(
	const DataSectionPtr pBoundarySection,
	PlaneEq& ret )
{
	Vector3 normal = pBoundarySection->readVector3( "normal", ret.normal() );
	float d = pBoundarySection->readFloat( "d", ret.d() );
	ret = PlaneEq( normal,d );
}


/**
 *	This method checks whether the two planes coincide, within a
 *	certain tolerance.
 */
bool portalOnBoundary( const PlaneEq& portal, const PlaneEq& boundary )
{
	//remember, portal normals face the opposite direction to boundary normals.
	const float normalTolerance = -0.999f;
	const float distTolerance = 0.0001f;	//distance squared

	if ( portal.normal().dotProduct( boundary.normal() ) < normalTolerance )
	{
		Vector3 p1( portal.normal() * portal.d() );
		Vector3 p2( boundary.normal() * boundary.d() );

		if ( (p1-p2).lengthSquared() < distTolerance )
			return true;
	}

	return false;
}


/**
 *	Go through our portals, and find the boundary they are on.
 *	Write the portals into the appropriate boundary sections.
 */
void exportPortalsToBoundaries( DataSectionPtr pVisualSection, BW::vector<VisualPortalPtr>& portals )
{
	if ( !portals.size() )
		return;

	// store the boundaries as plane equations
	BW::vector<DataSectionPtr> boundarySections;
	pVisualSection->openSections( "boundary", boundarySections );
	BW::vector<PlaneEq> boundaries;
	for ( size_t b=0; b < boundarySections.size(); b++ )
	{
		PlaneEq boundary;
		planeFromBoundarySection( boundarySections[b], boundary );
		boundaries.push_back( boundary );
	}


	Vector3 bbMin = pVisualSection->readVector3( "boundingBox/min" );
	Vector3 bbMax = pVisualSection->readVector3( "boundingBox/max" );
	Vector3 size = bbMax - bbMin;
	BoundingBox bb(bbMin, bbMax);

	// now look at all our portals, and assign them to a boundary
	bool eachOnBoundary = true;
	for (size_t i=0; i<portals.size(); ++i )
	{
		PlaneEq portalPlane;
		portals[i]->planeEquation( portalPlane );

		if ( bb.centre().dotProduct(portalPlane.normal()) - portalPlane.d() > 0.f)
		{
			// portal is facing outwards ...reversing
			portals[i]->reverseWinding();
			portals[i]->planeEquation( portalPlane );
		}

		bool found = false;
		for ( size_t b=0; b < boundaries.size(); b++ )
		{
			if ( portalOnBoundary(portalPlane,boundaries[b]) )
			{
				portals[i]->save( boundarySections[b] );
				found = true;
				break;	//continue with next visual portal
			}
		}

		if (!found)
			eachOnBoundary = false;
	}

	// Warn the user that one or more portals is not being exported
	if (!eachOnBoundary)
	{
		if ( ! LogMsg::automatedTest() )
		{
			::MessageBox(	0,
				L"One or more portals will not be exported since they are not on a boundary!",
				L"Portal error!",
				MB_ICONERROR );
		}
		melErrorMessages.addMultiLineString\
			("One or more portals will not be exported since they are not on a boundary!");
	}
}


/**
 *	This function searches through a node hierarchy looking for duplicate node
 *	names
 *	@param hierarchy the hierarchy to check
 *	@param names nodes found
 *	@param duplicates duplicate nodes found
 */
void findDuplicateNodes( Hierarchy& hierarchy, BW::set<BW::string>& names, 
					BW::set<BW::string>& duplicates )
{
	BW::string name = 
		hierarchy.name().substr( hierarchy.name().find_last_of( "|" ) + 1 );
	if (names.find( name ) != names.end())
	{
		duplicates.insert( name );
	}
	else
	{
		names.insert( name );
	}

	for( int i = 0; i < (int)hierarchy.children().size(); i++ )
	{
		findDuplicateNodes( hierarchy.child( hierarchy.children()[i] ), 
			names, duplicates );
	}
}


/**
 *	This function checks a node hierarchy for duplicate nodes and displays an
 *	error dialog if any duplicates are found.
 *	@param hierarchy the node hierarchy to check
 *	@return true if no errors were found
 */
bool checkHierarchy( Hierarchy& hierarchy )
{
	BW::set<BW::string> names;
	BW::set<BW::string> duplicates;
	names.insert( "Scene Root" );
	findDuplicateNodes( hierarchy, names, duplicates );

	if (duplicates.size())
	{
		char errorString[1024];
		BW::StringBuilder errorBuilder( errorString, 1024 );

		errorBuilder.appendf( 
			"The scene contains %d bones with duplicate names:\n", 
			duplicates.size() );


		for (BW::set<BW::string>::iterator it = duplicates.begin();
			it != duplicates.end(); ++it)
		{
			errorBuilder.append( it->c_str() );
			errorBuilder.append( '\n' );
		}

		if (!LogMsg::automatedTest())
		{
			::MessageBoxA( 0, errorBuilder.string(), 
				"Bone hierarchy validity checking", MB_ICONERROR );
		}
		melErrorMessages.addMultiLineString(errorBuilder.string());
		return false;
	}

	return true;
}

/**
 * This is a helper function to warn about duplicate mesh names
 * @param duplicates the duplicate mesh names
 */
void warnDuplicateMeshNames( const BW::set<BW::string>& duplicates )
{
	char errorString[1024];
	BW::StringBuilder errorBuilder( errorString, 1024 );

	errorBuilder.appendf( 
		"The scene contains %d meshes/skins with duplicate names:\n", 
		duplicates.size() );


	for (BW::set<BW::string>::iterator it = duplicates.begin();
		it != duplicates.end(); ++it)
	{
		errorBuilder.append( it->c_str() );
		errorBuilder.append( '\n' );
	}

	::MessageBoxA( 0, errorBuilder.string(), 
		"Model name validity checking", MB_ICONERROR );
	melErrorMessages.addMultiLineString(errorBuilder.string());
}

/**
 *	This method converts the filename into a resource name.  If the
 *	resource is not in the BW_RESOURCE_TREE returns false.
 *	Otherwise the method returns true and the proper resource name is
 *	returned in the resName.
 */
bool validResource( const BW::string& fileName, BW::string& retResourceName )
{
	BW::string resName = BWResolver::dissolveFilename( fileName );

	if (resName == "") return false;
	bool hasDriveName = (resName.c_str()[1] == ':');
	bool hasNetworkName = (resName.c_str()[0] == '/' && resName.c_str()[1] == '/');
	hasNetworkName |= (resName.c_str()[0] == '\\' && resName.c_str()[1] == '\\');
	if (hasDriveName || hasNetworkName)
		return false;

	retResourceName = resName;
	return true;
}


/**
 *	Display a dialog explaining the resource is invalid.
 */
void invalidExportDlg( const BW::string& fileName )
{
	BW::string errorString;
	errorString = BW::string( "The exported file \"" ) + fileName + BW::string( "\" is not in a valid game path.\n" );
	errorString += "Need to export to a *subfolder* of:";
    uint32 pathIndex = 0;
	while (BWResource::getPath(pathIndex) != BW::string(""))
	{
		errorString += BW::string("\n") + BWResource::getPath(pathIndex++);
	}
	if ( ! LogMsg::automatedTest() )
	{
		MessageBox( NULL, bw_utf8tow( errorString ).c_str(), 
				L"Visual/Animation Exporter Error", MB_OK | MB_ICONWARNING );
	}
	melErrorMessages.addMultiLineString(errorString);
}


/**
 *	Display a dialog explaining the resources is invalid.
 */
void invalidResourceDlg( const BW::vector<BW::string>& fileNames )
{
	BW::string errorString( "The following resources are not in a valid game path:   \n\n" );

	for (uint32 i=0; i<fileNames.size(); i++)
	{
		errorString += fileNames[i];
		errorString += "\n";
	}

	errorString += "\nAll resources must be from a *subfolder* of:    \n";
    uint32 pathIndex = 0;
	while (BWResource::getPath(pathIndex) != BW::string(""))
	{
		errorString += BW::string("\n") + BWResource::getPath(pathIndex++);
	}
	if ( !LogMsg::automatedTest() )
	{
		MessageBox( NULL, bw_utf8tow( errorString ).c_str(), 
			L"Visual/Animation Exporter Error", MB_OK | MB_ICONWARNING );
	}
	melErrorMessages.addMultiLineString(errorString);
}


/**
 *	Returns a map of visual names and bone counts.
 */
void VisualFileTranslator::getBoneCounts( BoneCountMap& boneCountMap )
{
	typedef BW::vector<VisualMeshPtr>::iterator visualMeshVectorIt;
	
	// If there are no visual meshes return zero
	if (visualMeshes.empty())
	{
		return;
	}

	// For each visual envelope, add its name and bone count
	visualMeshVectorIt beginIt = visualMeshes.begin();
	visualMeshVectorIt endIt = visualMeshes.end();
	for ( visualMeshVectorIt it = beginIt; it != endIt; ++it )
	{
		VisualEnvelope* visEnv =
			((*it)->isVisualEnvelope()) ?
				static_cast<VisualEnvelope*>(it->getObject()) :
				NULL;

		if (visEnv)
		{
			boneCountMap.insert(
				std::pair< BW::string, size_t >( 
					visEnv->getIdentifier(), visEnv->boneNodesCount() ) );
		}
	}
}


bool VisualFileTranslator::exportMesh( BW::string fileName )
{
	static int exportCount = 0;

	ScopedMELErrorVariableHandler melVariableErrorHandler;

	// remove extension from filename
	fileName = BWResource::removeExtension( fileName ) + ".visual";

	// Check if the filename is in the right res path.
	BW::string resName;
	if (!validResource( fileName, resName ))
	{
		invalidExportDlg( fileName );
		return false;
	}

	bool shell = isShell(resName);	// boolean for checking if this file will be a shell

	// Calculate a few useful file names
	char buf[256];
	bw_snprintf( buf, sizeof(buf), "exporter_%03d_temp.visual", exportCount++ );

	BW::string tempFileName = BWResource::getFilePath( fileName ) + buf;
	BW::string tempResName = BWResolver::dissolveFilename( tempFileName );
	BW::string tempPrimFileName = BWResource::removeExtension( tempFileName ) + ".primitives";
	BW::string tempPrimResName = BWResource::removeExtension( tempResName ) + ".primitives";
	BW::string primFileName = BWResource::removeExtension( fileName ) + ".primitives";
	BW::string primResName = BWResource::removeExtension( resName ) + ".primitives";
	BW::string visualBspFileName = BWResource::removeExtension( fileName ) + ".bsp";
	BW::string visualBspResName = BWResource::removeExtension( resName ) + ".bsp";

#if DEBUG_PRIMITIVE_FILE
	// Delete the previous primitive debug xml file is it exists
	BW::string primDebugXmlFileName = BWResource::removeExtension( fileName ) + ".primitives.xml";
	::DeleteFile( primDebugXmlFileName.c_str() );
#endif

	//Check all resources used by all VisualMeshes.
	BW::vector<BW::string> allResources;
	for (uint32 i=0; i<visualMeshes.size(); i++)
	{
		visualMeshes[i]->resources(allResources);
	}

	BW::vector<BW::string> invalidResources;
	for (uint32 i=0; i<allResources.size(); i++)
	{
		BW::string temp;
		if (!validResource( allResources[i], temp ))
		{
			invalidResources.push_back(allResources[i]);
		}
	}

	if (!invalidResources.empty())
	{
		invalidResourceDlg( invalidResources );
		return false;
	}

	BWResource::instance().purgeAll();

	// For keeping the current material kind setting.
	// Also, load up the existing visual file, if any.  We will try and use
	// the materials from the existing visual file if the ids match.  If there
	// is no existing file, no worries.
	DataSectionPtr pExistingVisual = BWResource::openSection( resName, false );

	BW::string materialKind;
	{
		if (pExistingVisual)
			materialKind = pExistingVisual->readString( "materialKind" );
	}

	if (!ExportSettings::instance().keepExistingMaterials())
		pExistingVisual = NULL;

	DataResource visualFile( tempFileName, RESOURCE_TYPE_XML );
	DataSectionPtr pVisualSection = visualFile.getRootSection();


	if (pVisualSection)
	{
		//------------------------------------
		//Export the nodes
		//------------------------------------
		pVisualSection->delChildren();
		Hierarchy hierarchy( NULL );

		// export hierarchy only for STATIC_WITH_NODES and NORMAL
		Mesh mesh;
		if ( ExportSettings::instance().exportMode() != ExportSettings::STATIC && ExportSettings::instance().exportMode() != ExportSettings::MESH_PARTICLES )
		{
			addAllHardpoints(hierarchy);
			addAllLocators(hierarchy);
			hierarchy.getMeshes( mesh );
		}
		Skin skin;

		skin.trim( mesh.meshes() );
		if( skin.count() > 0 && skin.initialise( 0 ) &&
			ExportSettings::instance().exportMode() != ExportSettings::STATIC && ExportSettings::instance().exportMode() != ExportSettings::MESH_PARTICLES )
		{

			hierarchy.getSkeleton( skin );
			for( uint32 i = 1; i < skin.count(); ++i )
				if( skin.initialise( i ) )
					hierarchy.getSkeleton( skin );

			for( uint32 j = 0; j < skin.count(); ++j )
			{
				skin.initialise( j );
				for( uint32 i = 0; i < skin.numberOfBones(); ++i )
				{
						hierarchy.find( BW::string( skin.boneDAG( i ).fullPathName().asChar() ) ).addNode( BW::string( skin.boneDAG( i ).fullPathName().asChar() ) + "_BlendBone", matrix4<float>()/*skin.transform( i, false ).inverse()*/ );
				}
			}
		}

		bool hasInvalidTransforms = false;
		exportTree( pVisualSection, hierarchy, "", &hasInvalidTransforms );
		if ( hasInvalidTransforms )
		{
			if ( ! LogMsg::automatedTest() )
			{
				::MessageBox(	0,
						L"The visual contains bones with invalid transforms (probably due to bone scaling)!\n"
						L"This may introduce animation artifacts inside BigWorld!",
						L"Invalid bone transform warning!",
						MB_ICONWARNING );
			}
			melErrorMessages.addMultiLineString("The visual contains bones with invalid transforms "\
												"(probably due to bone scaling)!");
		}
		if (!materialKind.empty())
			pVisualSection->writeString( "materialKind", materialKind );


		//~ //------------------------------------
		//~ //Export the primitives and vertices
		//~ //
		//~ //Calculate the bounding box while doing so
		//~ //------------------------------------


		BoundingBox bb = BoundingBox::s_insideOut_;
		VisualMeshPtr accMesh = new VisualMesh();

		// Remove the primitives file from disk to get rid of any unwanted sections in it
		::DeleteFile( bw_utf8tow( tempPrimFileName ).c_str() );

		BW::vector<VisualMeshPtr> origVisualMeshes = visualMeshes;

		// save out each mesh
		size_t meshCount = visualMeshes.size() + bspMeshes.size() + hullMeshes.size();
		int count = 0;
		while (visualMeshes.size())
		{
			if (ExportSettings::instance().exportMode() == ExportSettings::NORMAL)
			{

				if ( !visualMeshes.back()->save( pVisualSection, pExistingVisual, tempPrimFileName, true ) )
				{
					melErrorMessages.addMultiLineString(visualMeshes.back()->errorText());
					//return false;
				}

#if DEBUG_PRIMITIVE_FILE
				visualMeshes.back()->savePrimXml( primDebugXmlFileName );
#endif

				BoundingBox bb2 = visualMeshes.back()->bb();
				MDagPath meshDag = findDagPath( visualMeshes.back()->fullName() );

				MStatus stat;
				MFnTransform trans(meshDag.transform(), &stat );
				if (stat == MS::kSuccess)
				{
					Hierarchy* pNode = &hierarchy.find( trans.fullPathName().asChar() );
					if (&hierarchy != pNode)
					{
						matrix4<float> trans = hierarchy.transform(0, false).inverse() * pNode->transform( 0, false );
						bb2.transformBy( *(Matrix*)&trans );
					}
				}
				bb.addBounds( bb2 );
			}
			else
			{
				accMesh->dualUV( accMesh->dualUV() || visualMeshes.back()->dualUV() );
				accMesh->vertexColours( accMesh->vertexColours() || visualMeshes.back()->vertexColours() );

				bool worldSpaceAcc = ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES ? false : true;
				visualMeshes.back()->add( *accMesh, count++, worldSpaceAcc );
				bb.addBounds( visualMeshes.back()->bb() );
			}


			visualMeshes.pop_back();
		}
		

		if (ExportSettings::instance().exportMode() == ExportSettings::NORMAL)
		{
			Constraints::saveConstraints( pVisualSection );
		}


		// add bsps to the bb
		for (uint32 i = 0; i < bspMeshes.size(); i++)
		{
			bb.addBounds( bspMeshes[i]->bb() );
		}

		// add hulls to the bb
		for (uint32 i = 0; i < hullMeshes.size(); i++)
		{
			bb.addBounds( hullMeshes[i]->bb() );
		}

		// for mesh particles ensure there are 15 meshes
		if ( ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES )
		{
			int currentIndex = 0;
			while (count < 15 && !origVisualMeshes.empty())
			{
				// add mesh to acculated meshes
				origVisualMeshes[currentIndex++]->add( *accMesh, count, false );

				if (currentIndex == origVisualMeshes.size())
					currentIndex = 0;

				++count;
			}
		}

		bool saved=true;
		// save out accumulated mesh instead
		if ( ExportSettings::instance().exportMode() != ExportSettings::NORMAL )
		{
			if (count == 0)
			{
				BW::string errMsg(
					"No valid meshes were found to export.\n"
					"Export selection on a group node, "
					"a hull mesh or a bsp mesh is not supported.");
				if ( ! LogMsg::automatedTest() )
				{
					::MessageBox(	0,
								bw_utf8tow( errMsg ).c_str(),
								L"Export error!",
								MB_ICONERROR );
				}

				melErrorMessages.addMultiLineString( errMsg );
				accMesh = NULL;
				pVisualSection = NULL;
				::DeleteFile( bw_utf8tow( tempFileName ).c_str() );
				::DeleteFile( bw_utf8tow( tempPrimFileName ).c_str() );
				return false;
			}

			saved=accMesh->save( pVisualSection, pExistingVisual, tempPrimFileName, false );

#if DEBUG_PRIMITIVE_FILE
			accMesh->savePrimXml( primDebugXmlFileName );
#endif
		}
		if (meshCount==0 || !saved)
		{
			melErrorMessages.addMultiLineString(accMesh->errorText());
			accMesh = NULL;
			pVisualSection = NULL;
			::DeleteFile( bw_utf8tow( tempFileName ).c_str() );
			::DeleteFile( bw_utf8tow( tempPrimFileName ).c_str() );
			return false;
		}
		accMesh = NULL;

		bb.setBounds(	Vector3(	bb.minBounds().x,
									bb.minBounds().y,
									bb.minBounds().z ) *
									ExportSettings::instance().unitScale(),
						Vector3(	bb.maxBounds().x,
									bb.maxBounds().y,
									bb.maxBounds().z ) *
									ExportSettings::instance().unitScale() );
		pVisualSection->writeVector3( "boundingBox/min", bb.minBounds() );
		pVisualSection->writeVector3( "boundingBox/max", bb.maxBounds() );

		// export hull and portals
		// First remove all existing boundary sections from the visual
		DataSectionPtr pBoundary = pVisualSection->openSection( "boundary" );
		while ( pBoundary )
		{
			pVisualSection->delChild( pBoundary );
			pBoundary = pVisualSection->openSection( "boundary" );
		}

		// export hull/boundaries for shells
		if ( shell )
		{
			// support custom hulls
			if ( hullMeshes.empty() == false )
			{
				exportHull( pVisualSection, hullMeshes );
				pVisualSection->writeBool( "customHull", true );
			}
			else
			{
				generateHull(pVisualSection, bb);
				pVisualSection->writeBool( "customHull", false );
			}

			// Now write any explicitly set portal nodes.
			// The following method finds boundaries for our portals
			// and writes them into the appropriate boundary sections.
			exportPortalsToBoundaries( pVisualSection, visualPortals );
		}

		if ( ExportSettings::instance().exportMode() != ExportSettings::NORMAL && !bspMeshes.empty() )
			pVisualSection->writeBool( "customBsp", true );
	}

	visualFile.save();

	pVisualSection = NULL;

	BWResource::instance().purge( tempResName, true );
	BWResource::instance().purge( tempPrimResName, true );

	pVisualSection = BWResource::openSection( tempResName );

	if ( !ExportSettings::instance().disableVisualChecker() )
	{
		VisualChecker vc( tempFileName, false, ExportSettings::instance().snapVertices() );
		if( !vc.check( pVisualSection, tempPrimResName ) )
		{
			BW::string errs = vc.errorText();
			if (! LogMsg::automatedTest())
			{
				::MessageBox( 0, bw_utf8tow( errs ).c_str(), 
					L"Visual failed validity checking", MB_ICONERROR );
			}

			printf( errs.c_str() );
			printf( "\n" );
			fflush( stdout );

			::DeleteFile( bw_utf8tow( tempFileName ).c_str() );
			::DeleteFile( bw_utf8tow( tempPrimFileName ).c_str() );

			melErrorMessages.addMultiLineString(errs);

			return false;
		}
	}

	// Passed checking, rename to proper name
	::DeleteFile( bw_utf8tow( fileName ).c_str() );
	::DeleteFile( bw_utf8tow( primFileName ).c_str() );

	::MoveFile( bw_utf8tow( tempFileName ).c_str(), bw_utf8tow( fileName ).c_str() );
	::MoveFile( bw_utf8tow( tempPrimFileName ).c_str(), bw_utf8tow( primFileName ).c_str() );

	// Now process the BSP
	// Make sure there's no existing bsp file
	::DeleteFile( bw_utf8tow( visualBspFileName ).c_str() );

	//------------------------------------
	//Export the BSP
	//------------------------------------
	BW::vector<BW::string> bspMaterialIDs;
	// Export bsp tree (don't export bsps for MESH_PARTICLES).
	if ( ExportSettings::instance().exportMode() != ExportSettings::MESH_PARTICLES )
	{
		// Export bsp tree for all objects if there is a custom bsp tree defined.
		if ( !bspMeshes.empty() )
		{
			BW::string bspFileName = BWResource::removeExtension( fileName ) + "_bsp.visual";
			BW::string bspResName = BWResource::removeExtension( resName ) + "_bsp.visual";
			BW::string bspPrimFileName = BWResource::removeExtension( fileName ) + "_bsp.primitives";
			BW::string bspPrimResName = BWResource::removeExtension( resName ) + "_bsp.primitives";
			BW::string bspBspFileName = BWResource::removeExtension( fileName ) + "_bsp.bsp";

			DataResource bspFile( bspFileName, RESOURCE_TYPE_XML );

			DataSectionPtr pBSPSection = bspFile.getRootSection();

			if ( pBSPSection )
			{
				pBSPSection->delChildren();

				// Use same material kind as the real visual, so we get the correct data in the bsp
				if (!materialKind.empty())
					pBSPSection->writeString( "materialKind", materialKind );

				// bsp has only one node
				Hierarchy h( NULL );
				exportTree( pBSPSection, h );

				BoundingBox bb = BoundingBox::s_insideOut_;
				VisualMeshPtr accMesh = new VisualMesh();

				while ( bspMeshes.size() )
				{
					bspMeshes.back()->add( *accMesh );

					bb.addBounds( bspMeshes.back()->bb() );

					bspMeshes.pop_back();
				}

				if ( !accMesh->save( pBSPSection, pExistingVisual, bspPrimFileName, false ) )
				{
					melErrorMessages.addMultiLineString(accMesh->errorText());
				}
				accMesh = NULL;

				bb.setBounds(	Vector3(	bb.minBounds().x,
											bb.minBounds().z,
											bb.minBounds().y ),
								Vector3(	bb.maxBounds().x,
											bb.maxBounds().z,
											bb.maxBounds().y ) );
				pBSPSection->writeVector3( "boundingBox/min", bb.minBounds() * ExportSettings::instance().unitScale() );
				pBSPSection->writeVector3( "boundingBox/max", bb.maxBounds() * ExportSettings::instance().unitScale() );
			}

			bspFile.save();

			// Generate the bsp for the bsp visual
			generateBSP( bspResName, BWResource::removeExtension( bspResName ) + ".bsp", bspMaterialIDs );

			// Rename the .bsp file to the real visuals name
			::MoveFile( bw_utf8tow( bspBspFileName ).c_str(), bw_utf8tow( visualBspFileName ).c_str() );

			// Clean up the bsp visual
			::DeleteFile( bw_utf8tow( bspFileName ).c_str() );
			::DeleteFile( bw_utf8tow( bspPrimFileName ).c_str() );
		}
		// Only generate a bsp if the object is static,
		else if ( ExportSettings::instance().exportMode() != ExportSettings::NORMAL &&
			ExportSettings::instance().exportMode() != ExportSettings::NORMAL )
		{
			// Generate the bsp from the plain visual
			generateBSP( resName, visualBspResName, bspMaterialIDs );
		}

		// We may now have a .bsp file, move it into the .primitives file
		DataSectionPtr ds = BWResource::openSection( visualBspResName );
		BinaryPtr bp;

		if ( ds && (bp = ds->asBinary()) )
		{
			DataSectionPtr p = BWResource::openSection( primResName );
			if (p)
			{
				p->writeBinary( "bsp2", bp );

				DataSectionPtr materialIDsSection = 
					BWResource::openSection( "temp_bsp_materials.xml", true, XMLSection::creator() );
				materialIDsSection->writeStrings( "id", bspMaterialIDs );
				p->writeBinary( "bsp2_materials", materialIDsSection->asBinary() );

				p->save( primResName );
			}
			// Clean up the .bsp file
			::DeleteFile( bw_utf8tow( visualBspFileName ).c_str() );
		}
	}

	return true;
}


/**
 *	This class is a helper class to clean up the state of the 
 *	VisualFileTranslator after the writer method.
 */
class VisualFileTranslator::AutoCleanup
{
public:
	AutoCleanup(VisualFileTranslator& translator) :
		translator_( translator )
	{

	}
	~AutoCleanup()
	{
		translator_.cleanup();
	}
private:

	VisualFileTranslator& translator_;

	DataSectionCachePurger dscp;
	NodeCatalogueHolder nch;
};

/**
 *	This function parses a MString as a bool value
 *	it accepts integer values or string values
 *	it treats non-zero integer values as true
 *	and the string "True" as true
 *	@return the parsed bool value
 */
bool parseMStringBool(const MString& str)
{
	return ( (str.isInt() && (str.asInt() != 0))
	      || (str == "True") );
}


/**
 *	This function parses a MString as a ExportMode enum
 *	it accepts integer values or string values
 *  integer values are interpreted as 0=normal 1=static 
 *	2=static_with_nodes 3=mesh_particles
 *	it interprets the following string values:
 *	normal static static_with_nodes mesh_particles
 *	@return ExportMode enum value or EXPORTMODE_END if no value
 * 	is parsed
 */
ExportSettings::ExportMode parseMStringExportMode(const MString& str)
{
	ExportSettings::ExportMode returnValue = ExportSettings::EXPORTMODE_END;

	if (str.isInt())
	{
		int value = str.asInt();
		if (value >= 0 && value < ExportSettings::EXPORTMODE_END)
		{
			returnValue = (ExportSettings::ExportMode)value;
		}
	}
	else
	{
		MString strLower = MString(str).toLowerCase();
		if (strLower == "normal")
			returnValue = ExportSettings::NORMAL;
		else if (strLower == "static")
			returnValue = ExportSettings::STATIC;
		else if (strLower == "static_with_nodes")
			returnValue = ExportSettings::STATIC_WITH_NODES;
		else if (strLower == "mesh_particles")
			returnValue = ExportSettings::MESH_PARTICLES;
	}
	return returnValue;
}


/**
 *	This function parses a MString as a NodeFilter enum
 *	it accepts integer values or string values
 *  integer values are interpreted as 0=all 1=selected 2=visible
 *	it interprets the following string values:
 *	all selected visible
 *	@return NodeFilter enum value or NODEFILTER_END if no value
 * 	is parsed
 */
ExportSettings::NodeFilter parseMStringNodeFilter(const MString& str)
{
	ExportSettings::NodeFilter returnValue = ExportSettings::NODEFILTER_END;

	if (str.isInt())
	{
		int value = str.asInt();
		if (value >= 0 && value < ExportSettings::NODEFILTER_END)
		{
			returnValue = (ExportSettings::NodeFilter)value;
		}
	}
	else
	{
		MString strLower = MString(str).toLowerCase();
		if (strLower == "all")
			returnValue = ExportSettings::ALL;
		else if (strLower == "selected")
			returnValue = ExportSettings::SELECTED;
		else if (strLower == "visible")
			returnValue = ExportSettings::VISIBLE;
	}
	return returnValue;
}


/**
 * This function parses the Maya optionstring and sets the options
 * on the settings object
 * @param optionString the maya options string
 * @param settings the ExportSettings object to parse the settings
 * into
 */
void parseOptionsString( const MString& optionsString, ExportSettings& settings)
{	
	if (optionsString.length() > 0)
	{
		// Start parsing.
		MStringArray optionList;
		MStringArray theOption;
		optionsString.split(';', optionList);    // break out all the optionsString.

		for( unsigned int i = 0; i < optionList.length(); ++i )
		{
			theOption.clear();
			optionList[i].split( '=', theOption );
			if( theOption.length() > 1 )
			{
				if ( theOption[0] == "exportAnimation" )
				{
					settings.exportAnim(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "exportMode" )
				{
					ExportSettings::ExportMode exportMode = parseMStringExportMode(theOption[1]);
					if ( exportMode != ExportSettings::EXPORTMODE_END )
					{
						settings.exportMode(exportMode);
					}
				}
				else if ( theOption[0] == "boneCount" )
				{
					settings.maxBones(theOption[1].asInt());
				}
				else if ( theOption[0] == "transformToOrigin" )
				{
					settings.transformToOrigin(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "allowScale" )
				{
					settings.allowScale(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "bumpMapped" )
				{
					settings.bumpMapped(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "keepExistingMaterials" )
				{
					settings.keepExistingMaterials(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "snapVertices" )
				{
					settings.snapVertices(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "stripRefPrefix" )
				{
					settings.stripRefPrefix(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "useReferenceNode" )
				{
					settings.referenceNode(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "disableVisualChecker" )
				{
					settings.disableVisualChecker(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "useLegacyScaling" )
				{
					settings.useLegacyScaling(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "fixCylindrical" )
				{
					settings.fixCylindrical(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "useLegacyOrientation" )
				{
					settings.useLegacyOrientation(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "sceneRootAdded" )
				{
					settings.sceneRootAdded(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "includeMeshes" )
				{
					settings.setExportMeshes(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "includeEnvelopesAndBones" )
				{
					settings.setExportEnvelopesAndBones(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "includeNodes" )
				{
					settings.setExportNodes(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "includeMaterials" )
				{
					settings.setExportMaterials(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "includeAnimations" )
				{
					settings.setExportAnimations(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "useCharacterMode" )
				{
					settings.setUseCharacterMode(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "animationName" )
				{
					settings.setAnimationName(theOption[1].asChar());
				}
				else if ( theOption[0] == "includePortals" )
				{
					settings.setIncludePortals(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "worldSpaceOrigin" )
				{
					settings.setWorldSpaceOrigin(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "unitScale" )
				{
					settings.setUnitScale(theOption[1].asFloat());
				}
				else if ( theOption[0] == "localHierarchy" )
				{
					settings.setLocalHierarchy(parseMStringBool(theOption[1]));
				}
				else if ( theOption[0] == "nodeFilter" )
				{
					ExportSettings::NodeFilter nodeFilter = parseMStringNodeFilter(theOption[1]);
					if ( nodeFilter != ExportSettings::NODEFILTER_END )
					{
						settings.nodeFilter(nodeFilter);
					}
				}
				else if ( theOption[0] == "copyExternalTextures" )
				{
					settings.copyExternalTextures(
						parseMStringBool( theOption[1] ) );
				}
				else if ( theOption[0] == "copyTexturesTo" )
				{
					settings.copyTexturesTo( theOption[1].asChar() );
				}
				else
				{
					//std::cerr << "VisualFileTranslator export: unrecognised option '" << theOption[0] << "'." << std::endl;
				}
			}
		}
	}
}


// we overload this method to perform any
// file export operations needed
MStatus VisualFileTranslator::writer( const MFileObject& file, const MString& optionsString, FileAccessMode mode)
{
	AutoCleanup cleaner(*this);

	ScopedMELErrorVariableHandler scopedMELErrorVariableHandler;

	ExportSettings& settings = ExportSettings::instance();

	bool noPrompt = false;
	bool automatedTest = false;

	if (optionsString.length() > 0)
	{
		// Start parsing.
		MStringArray optionList;
		MStringArray theOption;
		optionsString.split(';', optionList);    // break out all the optionsString.

		for( unsigned int i = 0; i < optionList.length(); ++i )
		{
			theOption.clear();
			optionList[i].split( '=', theOption );
			if( theOption.length() > 1 )
			{
				if ( theOption[0] == "automatedTest" )
				{
					automatedTest = ( theOption[1].asInt() != 0 );
					LogMsg::automatedTest( automatedTest );
				}
				if ( theOption[0] == "noPrompt" )
				{
					noPrompt = ( theOption[1].asInt() != 0 );
				}
			}
		}
	}

	// Get the output filename
	const BW::string output_filename = BWResource::removeExtension( 
		bw_acptoutf8( file.fullName().asChar() ) ).to_string();

	const BW::string visual_filename = 
		BWResource::removeExtension( output_filename ) + ".visual";

	const BW::string visualsettings_filename = 
		BWResource::removeExtension( output_filename ) + ".visualsettings";

	const BW::string model_filename =
		BWResource::removeExtension( output_filename ) + ".model";

	const BW::string model_backupFileName =
		lookupBackupFilename( model_filename );

	BW::string resNameCamelCase;
	if (!validResource( visual_filename.c_str(), resNameCamelCase ))
	{
		invalidExportDlg( visual_filename.c_str() );
		return MS::kFailure;
	}
	
	VisualChecker vc( resNameCamelCase, false, settings.snapVertices() );
	BW::string vcErrors = vc.errorText();
	melErrorMessages.addMultiLineString(vcErrors);
	settings.visualTypeIdentifier( vc.typeName() );

	// Choose a default export type based on the visual type
	if (vc.exportAs() == "normal")
	{
		settings.exportMode( ExportSettings::NORMAL );
	}
	else if (vc.exportAs() == "static")
	{
		settings.exportMode( ExportSettings::STATIC );
	}
	else if (vc.exportAs() == "static with nodes")
	{
		settings.exportMode( ExportSettings::STATIC_WITH_NODES );
	}
	if( vc.typeName() == UNKNOWN_TYPE_NAME )
		settings.disableVisualChecker( true );

	// Populate the settings from the visual file.
	// If this fails, fall back to a visualsettings file at the destination,
	// if it exists.
	if (!settings.readSettings( lookupBackupFilename( visual_filename ), false ))
	{
		settings.readSettings( visualsettings_filename );
	}

	if ( !noPrompt )
	{
		VisualExporterDialog ved;
		if( ved.doModal( hInstance, GetForegroundWindow() ) == IDCANCEL )
			return MS::kSuccess;
	}
	
	settings.nodeFilter(ExportSettings::ALL);
	if( mode == kExportActiveAccessMode )
	{
		settings.nodeFilter(ExportSettings::SELECTED);

	}

	// Override any of the previously defined settings from the values in optionsString.
	parseOptionsString(optionsString, settings);

	BlendShapes blendShapes;

	Mesh mesh;

	Skin skin;

	Portal portal;

	MDagPathArray rootTransforms;
	if (settings.transformToOrigin())
	{
		MDagPathArray transforms;

		// Add all the meshes we are going to export
		for ( uint32 i = 0; i < mesh.count(); ++i )
		{
			MDagPath transform;
			MDagPath::getAPathTo( mesh.meshes()[i].transform(), transform );
			transforms.append( transform );
		}

		Mesh meshTemp;
		Skin skinTemp;

		skinTemp.trim( meshTemp.meshes() );
		meshTemp.trim( skinTemp.skins() );

		if (settings.exportMode() == ExportSettings::NORMAL ||
			settings.exportMode() == ExportSettings::STATIC_WITH_NODES)
		{
			// Add all the bones we are going to export
			for ( uint32 i = 0; i < skinTemp.count(); ++i )
			{
				MFnSkinCluster skin( skinTemp.skins()[i] );

				MDagPathArray influences;
				skin.influenceObjects( influences );

				for ( uint32 j = 0; j < influences.length(); ++j )
				{
					MDagPath transform;
					MDagPath::getAPathTo( influences[j].transform(), transform );
					transforms.append( transform );
				}
			}

			// Remove the meshes that will be transformed by the bones we
			// are going to export
			for ( uint32 i = 0; i < meshTemp.count(); ++i )
			{
				MDagPath transform;
				MDagPath::getAPathTo( meshTemp.meshes()[i].transform(), transform );
				for ( uint32 j = 0; j < transforms.length(); ++j )
				{
					if (transforms[j] == transform)
					{
						transforms.remove( j );
						break;
					}
				}
			}
		}

		// Filter the root transforms
		for ( uint32 i = 0; i < transforms.length(); ++i )
		{
			MString transformPath = transforms[i].fullPathName();
			uint32 transformPathLen = transformPath.length();

			bool isRoot = true;
			for ( uint32 j = rootTransforms.length(); j > 0; --j )
			{
				MString rootTransformPath = 
					rootTransforms[j - 1].fullPathName();
				uint32 rootTransformPathLen = rootTransformPath.length();
				if (strncmp(transformPath.asChar(), rootTransformPath.asChar(), 
					std::min( transformPathLen, rootTransformPathLen ) ) != 0)
				{
					continue;
				}

				if (transformPathLen >= rootTransformPathLen)
				{
					isRoot = false;
					break;
				}

				rootTransforms.remove( j );
			}

			if (isRoot)
			{
				rootTransforms.append( transforms[i] );
			}
		}
	}

	OriginTransformer originTransformer( rootTransforms );

	for( uint32 i = 0; i < mesh.count(); ++i )
	{
		// Disable the affect of BlendShapes
		BlendShapes::disable();

		bool bspNode = (toLower(mesh.meshes()[i].fullPathName().asChar()).find("_bsp") != BW::string::npos);
		if( mesh.initialise( i, (settings.exportMode() == ExportSettings::NORMAL
			|| settings.exportMode() == ExportSettings::MESH_PARTICLES ) && !bspNode ) == false )
			continue;

		if( mesh.name().substr( mesh.name().find_last_of( "|" ) + 1 ).substr( 0, 3 ) == "HP_" )
			continue;

		// FIX very inefficient - but too slow?
		bool skipMesh = false;

		// loop through all portals here
		for( uint32 j = 0; skipMesh == false && j < portal.count(); ++j )
		{
			portal.initialise( j );

			// do we have a portal mesh
			if ( mesh.meshes()[i].fullPathName().asChar() == portal.name() )
			{
				VisualPortalPtr spVisualPortal = new VisualPortal;

				if( spVisualPortal )
				{
					if( spVisualPortal->init( portal ) )
						visualPortals.push_back( spVisualPortal );
				}

				skipMesh = true;
			}
		}

		// check for a hull mesh
		bool hullNode = (toLower(mesh.meshes()[i].fullPathName().asChar()).find("_hull") != BW::string::npos);
		if ( hullNode && skipMesh == false )
		{
			// add the mesh to a hull node meshlist
			//~ printf( "Hull Mesh: %s\n", mesh.meshes()[i].fullPathName().asChar() );
			//~ fflush( stdout );
			HullMeshPtr spHullMesh = new HullMesh;
			spHullMesh->snapVertices(
				settings.exportMode() != ExportSettings::NORMAL &&
				ExportSettings::instance().snapVertices() );
			if( spHullMesh )
			{
				if( spHullMesh->init( mesh ) )
				{
					// add to hulls list
					hullMeshes.push_back( spHullMesh );
				}
				else
				{
					melErrorMessages.addMultiLineString(spHullMesh->errorText());
				}
			}

			skipMesh = true;
		}

		// check for bsp node
		if ( bspNode && skipMesh == false )
		{
			// add the mesh to a hull node meshlist
			//~ printf( "Bsp Mesh: %s\n", mesh.meshes()[i].fullPathName().asChar() );
			//~ fflush( stdout );
			VisualMeshPtr spVisualMesh = new VisualMesh;
			spVisualMesh->snapVertices(
				settings.exportMode() != ExportSettings::NORMAL &&
				ExportSettings::instance().snapVertices() );
			if( spVisualMesh )
			{
				if( spVisualMesh->init( mesh ) )
				{
					// add to hulls list
					bspMeshes.push_back( spVisualMesh );
				}
				else
				{
					melErrorMessages.addMultiLineString(spVisualMesh->errorText());
				}
			}

			skipMesh = true;
		}

		VisualEnvelopePtr spVisualEnvelope;

		bool skinned = false;
		// loop through all skin objects, only for NORMAL models
		for( uint32 j = 0; settings.exportMode() == ExportSettings::NORMAL && skipMesh == false && j < skin.count(); ++j )
		{
			skin.initialise( j );

			for( uint32 k = 0; skipMesh == false && k < skin.meshes().length(); ++k )
			{
				if( mesh.meshes()[i] == skin.meshes()[k] )
				{
					mesh.initialise(i, false);
					//~ printf( "Skinned: %s\n", mesh.meshes()[i].fullPathName().asChar() );
					//~ fflush( stdout );
					spVisualEnvelope = new VisualEnvelope;

					if( spVisualEnvelope )
					{
						if( !spVisualEnvelope->init( skin, mesh ) )
						{
							melErrorMessages.addMultiLineString(spVisualEnvelope->errorText());
							spVisualEnvelope = NULL;
						}
					}

					skipMesh = true;
					skinned = true;
				}
			}
		}

		if ( blendShapes.isBlendShapeTarget( mesh.meshes()[i].node() ) == false )
		{
			if ( skipMesh == true )
			{
				if  ( skinned == true )
				{
					// Use the visual envelope
					if( spVisualEnvelope )
					{
						if( blendShapes.hasBlendShape( mesh.meshes()[i].node() ) )
						{
							spVisualEnvelope->object = mesh.meshes()[i].node();
						}

						BW::vector<VisualEnvelopePtr> splitEnvelopes;

						if (spVisualEnvelope->split( ExportSettings::instance().maxBones(), splitEnvelopes ))
						{
							addMeshName( spVisualEnvelope->getIdentifier() );

							for (size_t i = 0; i < splitEnvelopes.size(); ++i)
							{
								visualMeshes.push_back( splitEnvelopes[i].get() );
							}
						}
					}
				}
			}
			else
			{
				//~ printf( "Mesh: %s\n", mesh.meshes()[i].fullPathName().asChar() );
				//~ fflush( stdout );
				VisualMeshPtr spVisualMesh = new VisualMesh;
				spVisualMesh->snapVertices(
					settings.exportMode() != ExportSettings::NORMAL &&
					ExportSettings::instance().snapVertices() );

				if( blendShapes.hasBlendShape( mesh.meshes()[i].node() ) )
					spVisualMesh->object = mesh.meshes()[i].node();

				if( spVisualMesh )
				{
					if( spVisualMesh->init( mesh ) )
					{
						addMeshName( spVisualMesh->getIdentifier() );
						visualMeshes.push_back( spVisualMesh );
					}
					else
					{
						melErrorMessages.addMultiLineString(spVisualMesh->errorText());
					}
				}
			}
		}
	}

	// Re-enable the affect of BlendShapes
	BlendShapes::enable();

	bool hasPortal = !visualPortals.empty();
	if (!hasPortal)
	{
		typedef BW::vector<HullMeshPtr>::const_iterator HullMeshIt;
		for( HullMeshIt iter = hullMeshes.cbegin();
			iter != hullMeshes.cend(); ++iter )
		{
			const HullMesh* mesh = (*iter).get();
			if (mesh->hasPortal())
			{
				hasPortal = true;
				break;
			}
		}
	}

	BW::string resName = BWResolver::dissolveFilename( output_filename.c_str() );
	if( !isShell( resName ) && hasPortal )
	{
		if ( ! LogMsg::automatedTest() )
		{
			if( MessageBox( NULL,
				L"You are exporting an object with portals into a non shell directory.\n"
				L"If you continue this object will not be recognised as a shell",
				L"Visual Exporter", MB_OKCANCEL | MB_ICONEXCLAMATION ) == IDCANCEL )
			{
				return MS::kFailure;
			}
		}else{
			LogMsg::logToFile(  "WARNING: You are exporting an object with portals "\
				"into a non shell directory. This object will not be recognised as a shell " );
		}
		melErrorMessages.addString("You are exporting an object with portals "\
									"into a non shell directory.\n");
	}

	if( !settings.exportAnim() )
	{
		if (ExportSettings::instance().exportMode() == ExportSettings::NORMAL &&
			duplicateMeshNames_.size())
		{
			warnDuplicateMeshNames( duplicateMeshNames_ );
			return MS::kFailure;
		}

		if( !exportMesh( output_filename.c_str() ) )
		{
			return MS::kFailure;
		}
	}

	// Xiaoming Shi : save model file, for bug 4993{
	if( ExportSettings::instance().exportMode() != ExportSettings::MESH_PARTICLES &&
		!settings.exportAnim() )
	{
		BW::string resName = BWResolver::dissolveFilename(  output_filename.c_str()  );
		DataResource modelFile( model_backupFileName, RESOURCE_TYPE_XML );

		DataSectionPtr pModelSection = modelFile.getRootSection();
		if (pModelSection)
		{
			MetaData::updateCreationInfo( pModelSection );
			pModelSection->deleteSections( "nodefullVisual" );
			pModelSection->deleteSections( "nodelessVisual" );
			BW::string filename = BWResource::removeExtension( resName ).to_string();
			if (ExportSettings::instance().exportMode() == ExportSettings::NORMAL ||
				ExportSettings::instance().exportMode() == ExportSettings::STATIC_WITH_NODES)
				pModelSection->writeString( "nodefullVisual", filename );
			else if (ExportSettings::instance().exportMode() == ExportSettings::STATIC)
				pModelSection->writeString( "nodelessVisual", filename );

			// Metadata
			char buffer[ MAX_COMPUTERNAME_LENGTH + 1 ];
			DWORD size = ARRAY_SIZE( buffer );
			BOOL res = GetComputerNameA( buffer, &size );
			char* computerName = res ? buffer : "Unknown";

			pModelSection->writeString( "metaData/sourceFile", MFileIO::currentFile().asChar() );
			pModelSection->writeString( "metaData/computer", computerName );
		}
		modelFile.save( model_filename );
	}
	// Xiaoming Shi : save model file, for bug 4993}

	if (settings.exportAnim())
	{
		bool errors = false;

		// Check if the filename is in the right res path.
		BW::string name = output_filename.c_str();
		BW::string name2 = BWResolver::dissolveFilename( name );
		if ((name2.length() == name.length()) || (name2.find_last_of("/\\") == BW::string::npos))
		{
			errors = true;
			BW::string errorString;
			errorString = BW::string( "The exported file \"" ) + name + BW::string( "\" is not in a valid game path.\n" );
			errorString += "Need to export to a *subfolder* of:";
			uint32 pathIndex = 0;
			while (BWResource::getPath(pathIndex) != BW::string(""))
			{
				errorString += BW::string("\n") + BWResource::getPath(pathIndex++);
			}
			if ( ! LogMsg::automatedTest() )
			{
				MessageBox( NULL, bw_utf8tow( errorString ).c_str(), 
							L"Animation Exporter Error", MB_OK | MB_ICONWARNING );
			}
			melErrorMessages.addMultiLineString(errorString);
			return MStatus::kFailure;
		}

		BW::string animName = name;
		size_t cropIndex = 0;
		if ((cropIndex = animName.find_last_of( "\\/" )) > 0)
		{
			cropIndex++;
			animName = animName.substr( cropIndex, animName.length() - cropIndex );
		}

		{
			// to make it save *.animations instead of *.visual.animations
			// when I give full file name in open file dialog
			// or overwrite existing file
			FILE* file2 = _wfopen( bw_utf8tow( ( BWResource::removeExtension( name ) + ".animation") ).c_str(), L"wb" );

#if DEBUG_ANIMATION_FILE
			// Delete the previous animation debug xml file if it exists
			BW::string animDebugXmlFileName = BWResource::removeExtension( name ) + ".animation.xml";
			::DeleteFile( animDebugXmlFileName.c_str() );
#endif

			if (file2)
			{
				Hierarchy hierarchy( NULL );
				Skin skin;

				BinaryFile animation( file2 );

				int nChannels = 0;

				skin.trim( mesh.meshes() );
				bool skeletalAnimation = skin.count() > 0 && skin.initialise( 0 );

				// Need to get meshes hierarchies first, then skeletal
				Mesh mesh;
				bool bMeshSkeletalAnimation = false;
				if (mesh.meshes().length() > 0)
				{
					bMeshSkeletalAnimation = true;
					hierarchy.getMeshes( mesh );
				}
				if (skeletalAnimation)
				{
					hierarchy.getSkeleton( skin );
					for (uint32 i = 1; i < skin.count(); ++i)
					{
						if (skin.initialise( i ))
						{
							hierarchy.getSkeleton( skin );
						}
					}
				}
				if (bMeshSkeletalAnimation || skeletalAnimation)
				{
					nChannels += hierarchy.count();
					skeletalAnimation = true;
				}

				BW::string referenceNodesFile =
					ExportSettings::instance().referenceNodesFile();
				if (ExportSettings::instance().referenceNode() && !noPrompt)
				{
					wchar_t fileName[1025] = { 0 };
					OPENFILENAME openFileName = {
						sizeof( OPENFILENAME ),
						GetForegroundWindow(),
						0,
						L"Visual Files\0*.visual\0All Files\0*.*\0\0",
						NULL,
						0,
						0,
						fileName,
						1024,
						NULL,
						0,
						NULL,
						NULL,
						OFN_LONGNAMES
					};
					if (GetOpenFileName( &openFileName ))
					{
						referenceNodesFile = bw_wtoutf8( fileName );
						ExportSettings::instance().referenceNodesFile( referenceNodesFile );
					}
				}
				if (!referenceNodesFile.empty())
				{
					this->loadReferenceNodes( referenceNodesFile );
					StringHashMap< BW::string >::iterator it = nodeParents_.begin();
					StringHashMap< BW::string >::iterator end = nodeParents_.end();
					while (it != end)
					{
						BW::string parent = it->second;
						BW::string child = it->first;

						Hierarchy* p = hierarchy.recursiveFind( parent );
						Hierarchy* c = hierarchy.recursiveFind( child );
						if ((p && c) && (p != c->getParent()))
						{
							// 1. check if in the same branch
							{
								Hierarchy* parent = p;
								while (parent && parent->getParent() != c)
								{
									parent = parent->getParent();
								}
								if (parent)
								{
									BW::string name = c->removeNode( parent );
									if (name.size())
									{
										c->getParent()->addNode( name, *parent );
									}
								}
							}

							// 2. move
							Hierarchy* op = c->getParent();
							if (op)
							{
								op->removeNode( child );
							}
							p->addNode( child, *c );
						}
						
						++it;
					}
				}

				BlendShapes blendShapes;

				for( uint32 i = 0; i < blendShapes.count(); ++i )
				{
					blendShapes.initialise( i );

					// TODO : fix, only doing base 0 for now
					if (blendShapes.countTargets() > 0)
					{
						nChannels += blendShapes.numTargets( 0 );
					}
				}

				if (!ExportSettings::instance().sceneRootAdded())
				{
					++nChannels; // Plus one for the "Scene Root" (see below)
				}

				animation << float( hierarchy.numberOfFrames() - 1 ) << animName << animName;
				animation << nChannels;

#if DEBUG_ANIMATION_FILE
				exportAnimationXmlHeader( animDebugXmlFileName, animName, float( hierarchy.numberOfFrames() - 1 ), nChannels );
#endif

				if( skeletalAnimation )
				{
					if (!checkHierarchy( hierarchy ))
					{
						return MS::kFailure;
					}

					// Special requirement for the root node.  The node must be named for
					// it to be exported.
					if (!ExportSettings::instance().sceneRootAdded())
					{
						hierarchy.name("Scene Root");
						hierarchy.customName("Scene Root");
					}
					exportAnimation( animation, hierarchy, ExportSettings::instance().stripRefPrefix() );

#if DEBUG_ANIMATION_FILE
					DataSectionPtr spFileRoot = BWResource::openSection( animDebugXmlFileName );
					if ( spFileRoot )
					{
						DataSectionPtr spFile = spFileRoot->openSection( animName );
						
						if ( spFile )
							exportAnimationXml( spFile, hierarchy, ExportSettings::instance().stripRefPrefix() );
					}
					spFileRoot->save( animDebugXmlFileName );
#endif
				}

				fclose(file2);
			}
		}
	}

	if (settings.exportAnim())
	{
		settings.writeSettings( visualsettings_filename );
	}
	else
	{
		settings.writeSettings( visual_filename, false );
	}
	// writing of file was sucessful
	return MS::kSuccess;
}


/**
 * This method cleans up temporary data used while exporting.
 * It should be called before exiting from the writer method, this is 
 * currently achieved through the AutoCleanup class
 */
void VisualFileTranslator::cleanup()
{
	visualMeshes.clear();
	hullMeshes.clear();
	bspMeshes.clear();
	visualPortals.clear();
	meshNames_.clear();
	duplicateMeshNames_.clear();
}


/**
 *	This method registers a unique mesh name
 *	It keeps a list of mesh names that have already been used
 *	and a separate list of meshnames that are duplicates.
 *	The duplicate mesh name list is used to report an error when multiple meshes
 *	have duplicate names.
 */
void VisualFileTranslator::addMeshName( const BW::string& name )
{
	if (meshNames_.find(name) == meshNames_.end())
	{
		meshNames_.insert( name );
	}
	else
	{
		duplicateMeshNames_.insert( name );
	}
}


// returns true if this class can export files
bool VisualFileTranslator::haveWriteMethod() const
{
	return true;
}


// returns the default extension of the file supported
// by this FileTranslator.
MString VisualFileTranslator::defaultExtension() const
{
	return "BigWorld";
}

// add a filter for open file dialog
MString VisualFileTranslator::filter() const
{
	// Maya 6.5 doesn't support extension name that is longer than 6
	// like "visual", otherwise it crashes randomly, need further investigation
	return "*.*";
}

// Used by Maya to create a new instance of our
// custom file translator
void* VisualFileTranslator::creator()
{
	return new VisualFileTranslator;
}

void VisualFileTranslator::loadReferenceNodes( const BW::string & visualFile )
{
	BW::string filename = BWResolver::dissolveFilename( visualFile );

	DataSectionPtr pVisualSection = BWResource::openSection( filename );

	if (pVisualSection)
	{
		readReferenceHierarchy( pVisualSection->openSection( "node" ) );
	}
}

void VisualFileTranslator::readReferenceHierarchy( DataSectionPtr hierarchy )
{
	BW::string id = hierarchy->readString( "identifier" );
	BW::vector< DataSectionPtr> nodeSections;
	hierarchy->openSections( "node", nodeSections );

	for (uint i = 0; i < nodeSections.size(); i++)
	{
		DataSectionPtr p = nodeSections[i];
		nodeParents_[ p->readString( "identifier" ) ] = id;
		readReferenceHierarchy( p );
	}
}

BW::string VisualFileTranslator::lookupBackupFilename( const BW::string& fileName )
{
	if (BWResource::fileExists( fileName ))
	{
		return fileName;
	}

	IFileSystem& system = *BWResource::instance().nativeFileSystem();
	IFileSystem::Directory contents;
	BW::string directory = BWResource::getFilePath( fileName );

	if (!system.readDirectory( contents, directory ))
	{
		return fileName;
	}

	BW::string shortFileName = BWResource::getFilename( fileName ).to_string();
	BW::string filePath = BWResource::getFilePath( fileName );
	BW::string fileFound = fileName;
	size_t length = shortFileName.length();

	for (BW::string& file: contents)
	{
		if (file.length() <= length)
		{
			continue;
		}

		BW::string filePrefix = file.substr( 0, length );

		if (filePrefix != shortFileName || file[length] == '.')
		{
			continue;
		}

		if (fileFound != fileName)
		{
			MString warning = "Multiple backup files were found, ";
			warning +=
				"please clean these up to preserve the data when exporting.";
			MGlobal::displayWarning( warning );
			return fileName;
		}

		fileFound = filePath + file;
	}

	return fileFound;
}

BW_END_NAMESPACE

