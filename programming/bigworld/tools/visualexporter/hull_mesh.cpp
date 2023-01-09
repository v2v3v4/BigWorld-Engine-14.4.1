#include "hull_mesh.hpp"
#include "utility.hpp"
#include "cstdmf/debug.hpp"
#include "math/lineeq3.hpp"
#include <string>

DECLARE_DEBUG_COMPONENT2( "VisualExporter", 0 )

BW_BEGIN_NAMESPACE

// Private utility structure used to track edge indices
HullMesh::BiIndex::BiIndex() : i1(0u), i2(0u) {}

HullMesh::BiIndex::BiIndex(uint i1, uint i2) : i1(i1), i2(i2) {}

static const float normalTolerance = 0.99f;
static const float distTolerance = 0.25f;


uint& HullMesh::BiIndex::operator[](uint index)
{
	index = index % 2;
	if (index == 0)
		return i1;
	else
		return i2;
}

const uint& HullMesh::BiIndex::operator[](uint& index)
{
	index = index % 2;
	if (index == 0)
		return i1;
	else
		return i2;
}

// Private utility structure used to track triangle indices
HullMesh::TriIndex::TriIndex() : i1(0u), i2(0u), i3(0u) {}

HullMesh::TriIndex::TriIndex(uint i1, uint i2, uint i3) : i1(i1), i2(i2), i3(i3) {}

uint& HullMesh::TriIndex::operator[](uint index)
{
	index = index % 3;
	if (index == 0)
		return i1;
	else if (index == 1)
		return i2;
	else 
		return i3;
}

const uint& HullMesh::TriIndex::operator[](uint& index)
{
	index = index % 3;
	if (index == 0)
		return i1;
	else if (index == 1)
		return i2;
	else 
		return i3;
}


/**
 *	This method saves all of the boundaries
 *	and portals to the given data section.
 *
 *	It outputs a series of <boundary> tags,
 *	and if a boundary has an associated portal,
 *	the boundary tag will have a <portal> child.
 */
void HullMesh::save( DataSectionPtr pSection )
{
	//Convert all vertices etc. to correct units + orientation.
	VertexVector vv;
	this->createVertexList( vv );

	// Make sure the hull in convex
	BW::vector<TriIndex> faces;
	if ( !this->makeConvex( vv, faces ) )
	{
#ifdef _UNICODE
		::MessageBox(	0,
			L"Unable to create custom hull.  Custom hull most contain at least four "
			L"non-colinear and non-coplanar vertices!",
			L"Custom hull error!",
			MB_ICONERROR );
#else
		::MessageBox(	0,
			"Unable to create custom hull.  Custom hull most contain at least four "
			"non-colinear and non-coplanar vertices!",
			"Custom hull error!",
			MB_ICONERROR );
#endif	
		return;
	}

	//Convert our triangular mesh into boundary planes
	//and associated portals.
	this->extractPlanesAndPortals( vv, faces );

	// Need to track the number of portals added since the converting
	// of the custom hull to a convex hull may have moved the hull where
	// a portal was previously defined.
	size_t portalsAdded = portals_.size();
	for ( size_t i=0; i<planeEqs_.size(); i++ )
	{
		PlaneEq& plane = planeEqs_[i];
		bool isPortal = false;
		
		DataSectionPtr pBoundary = pSection->newSection( "boundary" );
		pBoundary->writeVector3( "normal", plane.normal() );
		pBoundary->writeFloat( "d", plane.d() );

		PortalMap::iterator it = portals_.find(static_cast<int>(i));
		if ( it != portals_.end() )
		{
			VisualPortalPtr vpp = it->second;
			vpp->save( pBoundary );	
			portalsAdded--;
		}
	}

	// Check that all portals were added
	if (portalsAdded != 0)
#ifdef _UNICODE
		::MessageBox(	0,
		L"One or more portals defined using hull materials will not be exported.\n"
		L"The hull was converted from concave to convex at that location!",
		L"Portal error!",
		MB_ICONERROR );
#else
		::MessageBox(	0,
		"One or more portals defined using hull materials will not be exported.\n"
		"The hull was converted from concave to convex at that location!",
		"Portal error!",
		MB_ICONERROR );
#endif
		
}

/**
 *	Check if the HullMesh has a portal.
 *  In addition to checking the size of portals_ also check for the 
 *  materials assigned to triangles. This second check could be done 
 *  in the case of calling hasPortals before portals_ has been 
 *  populated by the export functions.
 *  
 */
bool HullMesh::hasPortal() const
{
	if (portals_.size())
	{
		return true;
	}

	typedef TriangleVector::const_iterator TriIt;

	for ( TriIt it = triangles_.cbegin(); it != triangles_.cend(); ++it )
	{
		const Triangle& tri = *it;
		if (isPortal( tri.materialIndex ))
		{
			return true;
		}
	}

	return false;
}

/**
 *	This method goes through all the triangles in the hull,
 *	and extracts a set of plane equations out.
 *
 *	It also stores the indices of planes that are designated
 *	portals via their materials.
 */
void HullMesh::extractPlanesAndPortals( VertexVector& vv, BW::vector<TriIndex>& faces )
{
	planeEqs_.clear();
	portals_.clear();

	BW::vector<TriIndex>::iterator face_it;
	for ( face_it = faces.begin(); face_it != faces.end(); face_it++ )
	{
		// Add face plane (note that the winding needs to be reversed)
		PlaneEq peq(
			Vector3(vv[face_it->i3].pos_),
			Vector3(vv[face_it->i2].pos_),
			Vector3(vv[face_it->i1].pos_) );

		int planeIndex = addUniquePlane( peq );
	}

	// Loop through the original triangles searching for portal entries.  It is
	// safe to add them even though the hull may have been altered in makeConvex
	// since the portals will be tested against the hull before saving
	TriangleVector::iterator it = triangles_.begin();
	TriangleVector::iterator end = triangles_.end();

	while ( it != end )
	{
		Triangle& tri = *it++;
		PlaneEq peq(
			Vector3(vv[tri.index[0]].pos_),
			Vector3(vv[tri.index[1]].pos_),
			Vector3(vv[tri.index[2]].pos_) );

		int planeIndex = findPlane(peq);

		if (isPortal( tri.materialIndex ))
		{
			if ( portals_.find(planeIndex) == portals_.end() )
			{
				VisualPortalPtr vpp = new VisualPortal;
				portals_.insert( std::make_pair(planeIndex,vpp) );

				//Set the name of the portal, based on the material name.
				Material& mat = materials_[tri.materialIndex];
				BW::string matName = toLower(mat.identifier);
				size_t nPos = matName.find("_portal");
				matName = matName.substr(0,nPos);
				const char *dataName = vpp->propDataFromPropName(matName.c_str());
				if ( dataName )
					vpp->name(dataName);
			}

			VisualPortalPtr& vpp = portals_.find(planeIndex)->second;
			vpp->addSwizzledPoint(Vector3(vv[tri.index[2]].pos_));
			vpp->addSwizzledPoint(Vector3(vv[tri.index[1]].pos_));
			vpp->addSwizzledPoint(Vector3(vv[tri.index[0]].pos_));
		}
	}


	//Create convex hulls from all the VisualPortals
	PortalMap::iterator pit = portals_.begin();
	while ( pit != portals_.end() )
	{
		VisualPortalPtr vpp = pit->second;
		vpp->createConvexHull();
		pit++;
	}
}


/**
 *	This method search for a face that matches the passed parameter in the triangles_ vector.
 *	
 *	@param face			The face being searched for.
 *	@returns			A pointer to the original triangle if its found, NULL otherwise.
 */
VisualMesh::Triangle* HullMesh::findOriginalTriangle( TriIndex& face )
{
	TriangleVector::iterator it;
	for ( it = triangles_.begin(); it != triangles_.end(); it++ )
	{
		for ( int i = 0; i < 3; i++ )
		{
			if (it->index[2] == face[i] &&
				it->index[1] == face[i+1] &&
				it->index[0] == face[i+2])		// Note that the winding orders are different
				return &*it;
		}
	}

	return NULL;
}


/**
 *	This method searches our current planes and sees
 *	if the given plane is already in our database.
 *	If so, the index of the existing plane is returned.
 *	If the given plane is unique, we add it, and
 *	return the index of this new plane.
 */
int HullMesh::addUniquePlane( const PlaneEq& peq )
{
	for ( size_t planeIdx = 0; planeIdx < planeEqs_.size(); planeIdx++ )
	{
		PlaneEq& existing = planeEqs_[planeIdx];

		if ( existing.normal().dotProduct(peq.normal()) > normalTolerance )
		{
			if ( fabs(existing.d()-peq.d()) < distTolerance )
			{
				return static_cast<int>(planeIdx);
			}
		}
	}

	planeEqs_.push_back( peq );
	return static_cast<int>(planeEqs_.size() - 1);
}


/**
 *	This method searches our current planes and sees
 *	if the given plane is in our database.
 *	If so, the index of the existing plane is returned.
 *	Otherwise -1 is returned
 */
int HullMesh::findPlane( const PlaneEq& peq )
{
	for ( size_t planeIdx = 0; planeIdx < planeEqs_.size(); planeIdx++ )
	{
		PlaneEq& existing = planeEqs_[planeIdx];

		if ( existing.normal().dotProduct(peq.normal()) > normalTolerance )
		{
			if ( fabs(existing.d()-peq.d()) < distTolerance )
			{
				return static_cast<int>(planeIdx);
			}
		}
	}

	return -1;
}


/**
 *	Returns true if the material given by material index is a
 *	portal.  Currently this test involves checking if the word
 *	"portal" is contained in the material identifier.
 */
bool HullMesh::isPortal( uint materialIndex ) const
{
	MF_ASSERT( (size_t)materialIndex < materials_.size() )
	const Material& mat = materials_[materialIndex];
	return (toLower(mat.identifier).find("_portal") != BW::string::npos);
}


/**
 *	This method creates a convex hull from a vector of vertices.  It will determine if the vertices
 *	describe a convex or a concave hull.  If they describe a concave hull, a convex hull that
 *	enclose this cancave hull is created and a warning is displayed.
 *
 *	@param vertices		The list of hull vertices.
 *	@returns			Convex hull created
 */
bool HullMesh::makeConvex( VertexVector& vertices, BW::vector<TriIndex>& faces ) const
{
	// Flag used to record whether the hull was initially concave
	bool isConcave = false;

	// Initialise vectors
	BW::vector<uint> verts;			// The unresolved verts
	for ( size_t i = 0; i < vertices.size(); i++ )
		// Initialise the verts list with all vertices
		verts.push_back( static_cast<uint>(i) );

	// Create the maximal volume tetrahedron
	if ( !this->mcCreateMaxTetrahedron( vertices, verts, faces ) )
		// Cannot created convex hull
		return false;

	// Add next vert
	while ( verts.size() )
	{
		// Remove any points inside the volume
		if ( this->mcRemoveInsideVertices( vertices, verts, faces ) )
		{
			isConcave = true;

			if (verts.size() == 0)
				break;
		}

		// Find the vert that is furthest from the hull
		uint vertIndex = this->mcFindNextVert( vertices, verts, faces );

		// Create new faces using this furthest vertex
		this->mcExpandHull( vertIndex, vertices, verts, faces );
	}

	// Check if hull was concave
	if ( isConcave )
	{
		if ( LogMsg::automatedTest() )
		{
			LogMsg::logToFile( "WARNING: The custom hull is concave "\
					"or has colinear/coplanar vertices. The exported hull may "\
					"have changed shape to make it convex!\n");
		}
		else
		{
#ifdef _UNICODE
			::MessageBox(	0,
				L"The custom hull is concave or has colinear/coplanar vertices. The exported hull may have changed shape to make it convex!",
				L"Custom hull warning!",
				MB_ICONWARNING );
#else
			::MessageBox(	0,
				"The custom hull is concave or has colinear/coplanar vertices. The exported hull may have changed shape to make it convex!",
				"Custom hull warning!",
				MB_ICONWARNING );
#endif
		}
	}

	// Successfully created convex hull
	return true;
}


/**
 *	This method creates the Maximal Volume Tetrahedron (MVT).  The MVT is the maximum volume
 *	enclosed by any four points of the vertices point cloud.
 *
 *	@param vertices		The vertices to be enclosed in the convex hull.
 *	@param verts		The verts outside the convex hull.
 *	@param faces		The faces of the convex hull.
 *	@returns			MVT created
 */
bool HullMesh::mcCreateMaxTetrahedron(const VertexVector& vertices, BW::vector<uint>& verts,
										BW::vector<TriIndex>& faces, const float epsilon ) const
{
	// Must be at least four vertices
	if ( (vertices.size() < 4) || (verts.size() < 4) )
		return false;

	// Find the six points at the extremes of each axis
	float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
	float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
	uint minX_index, minY_index, minZ_index,
		 maxX_index, maxY_index, maxZ_index;
	BW::vector<uint>::iterator it;
	for ( it = verts.begin(); it != verts.end(); it++ )
	{
		if ( vertices[*it].pos_[0] < minX ) { minX = vertices[*it].pos_[0]; minX_index = *it; }
		if ( vertices[*it].pos_[1] < minY ) { minY = vertices[*it].pos_[1]; minY_index = *it; }
		if ( vertices[*it].pos_[2] < minZ ) { minZ = vertices[*it].pos_[2]; minZ_index = *it; }
		if ( vertices[*it].pos_[0] > maxX ) { maxX = vertices[*it].pos_[0]; maxX_index = *it; }
		if ( vertices[*it].pos_[1] > maxY ) { maxY = vertices[*it].pos_[1]; maxY_index = *it; }
		if ( vertices[*it].pos_[2] > maxZ ) { maxZ = vertices[*it].pos_[2]; maxZ_index = *it; }
	}

	// Find the longest extent out of the three axes.  This will form one of
	// the edges of the MVT.  Remove these indices from verts
	BiIndex edge;
	LineEq3 line;
	if ( (maxX - minX) > (maxY - minY) )
		if ( (maxX - minX) > (maxZ - minZ) )
		{
			edge = BiIndex(minX_index, maxX_index);
			line = LineEq3(Vector3(vertices[minX_index].pos_),
							Vector3(vertices[maxX_index].pos_));
		}
		else
		{
			edge = BiIndex(minZ_index, maxZ_index);
			line = LineEq3(Vector3(vertices[minZ_index].pos_),
							Vector3(vertices[maxZ_index].pos_));
		}
	else
		if ( (maxY - minY) > (maxZ - minZ) )
		{
			edge = BiIndex(minY_index, maxY_index);
			line = LineEq3(Vector3(vertices[minY_index].pos_),
							Vector3(vertices[maxY_index].pos_));
		}
		else
		{
			edge = BiIndex(minZ_index, maxZ_index);
			line = LineEq3(Vector3(vertices[minZ_index].pos_),
							Vector3(vertices[maxZ_index].pos_));
		}
	this->mcRemoveVertex( edge.i1, vertices, verts );
	this->mcRemoveVertex( edge.i2, vertices, verts );

	// Must be at least two verts left and line must be valid (i.e. cannot be
	// constructed from coincident points)
	if ( (verts.size() < 2) || !line.isValid() )
		return false;

	// Find the point with the greatest perpindicular distance from edge.
	// Remove the index from verts
	float dist = -FLT_MAX;
	uint index;
	for ( it = verts.begin(); it != verts.end(); it++ )
	{
		if (  line.perpDistance(Vector3(vertices[*it].pos_)) > dist )
		{
			dist = line.perpDistance(Vector3(vertices[*it].pos_));
			index = *it;
		}
	}
	this->mcRemoveVertex( index, vertices, verts );

	// Must be at least one vert left and must have a point that is not colinear to line
	if (  (verts.size() < 1) || (dist < epsilon) )
		return false;

	TriIndex triangle(edge.i1, edge.i2, index);

	// Create a plane from the three points
	PlaneEq plane(	Vector3(vertices[triangle.i1].pos_),
					Vector3(vertices[triangle.i2].pos_),
					Vector3(vertices[triangle.i3].pos_));

	// Find the point that is furthest from the plane
	dist = -FLT_MAX;
	for ( it = verts.begin(); it != verts.end(); it++ )
	{
		float tempDist = plane.distanceTo(Vector3(vertices[*it].pos_));

		if ( abs( tempDist ) > dist )
		{
			dist = abs( tempDist );
			index = *it;
		}
	}
	this->mcRemoveVertex( index, vertices, verts );

	// Must have a point that is not coplanar to plane
	if ( dist < epsilon )
		return false;

	// Create the MVT from the four identified points
	if ( plane.distanceTo(Vector3(vertices[index].pos_)) > 0 )
	{
		// Reverse the order of the verts when adding the face
		faces.push_back( TriIndex( triangle.i3, triangle.i2, triangle.i1 ) );

		// Add remaining 3 faces in reverse order
		faces.push_back( TriIndex( triangle.i1, triangle.i2, index ) );
		faces.push_back( TriIndex( triangle.i2, triangle.i3, index ) );
		faces.push_back( TriIndex( triangle.i3, triangle.i1, index ) );
	}
	else
	{
		// Add as defined
		faces.push_back( triangle );

		// Add remaining 3 faces
		faces.push_back( TriIndex( triangle.i3, triangle.i2, index ) );
		faces.push_back( TriIndex( triangle.i2, triangle.i1, index ) );
		faces.push_back( TriIndex( triangle.i1, triangle.i3, index ) );
	}

	// MVT created successfully
	return true;
}


/**
 *	This method removes a vertex from the verts list as well as all vertices with the
 *	same position within the epsilon threshold.
 *
 *	@param vertIndex	The index of the vertex to remove from verts.
 *	@param vertices		The vertices to be enclosed in the convex hull.
 *	@param verts		The verts outside the convex hull.
 *	@param epsilon		The threshold determining if two vertices are coincident.
 */
void HullMesh::mcRemoveVertex(const uint vertIndex, const VertexVector& vertices,
								BW::vector<uint>& verts, const float epsilon ) const
{
	// Remove all vertices at this position
	Vector3 removePosition( vertices[vertIndex].pos_ );

	BW::vector<uint>::iterator it;
	for ( it = verts.begin(); it != verts.end(); )
	{
		Vector3 diff = Vector3( vertices[*it].pos_ ) - removePosition;
		if ( diff.length() < epsilon )
			it = verts.erase( it );
		else
			it++;
	}
}


/**
 *	This method removes any vertices contained inside the convex hull.
 *
 *	@param vertices		The vertices to be enclosed in the convex hull.
 *	@param verts		The verts outside the convex hull.
 *	@param faces		The faces of the convex hull.
 *	@returns			Removed one or more vertices.
 */
bool HullMesh::mcRemoveInsideVertices(const VertexVector& vertices, BW::vector<uint>& verts,
										const BW::vector<TriIndex>& faces ) const
{
	bool vertsRemoved = false;

	// Loop through the verts
	BW::vector<uint>::iterator it;
	for ( it = verts.begin(); it != verts.end(); )
	{
		// Loop through the faces, checking the vert against each face.
		// If the vert is on the outside of any face, it must be outside
		// the convex hull
		bool inside = true;

		BW::vector<TriIndex>::const_iterator face_it;
		for ( face_it = faces.begin(); face_it != faces.end(); face_it++ )
		{
			// If on positive side, must be outside
			if ( PlaneEq(	Vector3( vertices[face_it->i1].pos_ ),
							Vector3( vertices[face_it->i2].pos_ ),
							Vector3( vertices[face_it->i3].pos_ ))
					.isInFrontOfExact( Vector3( vertices[*it].pos_ ) ) )
			{
				inside = false;
				break;
			}
		}

		if (inside)
		{
			vertsRemoved = true;
			this->mcRemoveVertex( *it, vertices, verts );
		}
		else
		{
			it++;
		}
	}

	return vertsRemoved;
}


/**
 *	This method returns the index of the next vertex to be add to the convex hull.
 *
 *	@param vertices		The vertices to be enclosed in the convex hull.
 *	@param verts		The verts outside the convex hull.
 *	@param faces		The faces of the convex hull.
 *	@returns			The index of the next vertex to be added.
 */
uint HullMesh::mcFindNextVert(const VertexVector& vertices, const BW::vector<uint>& verts,
								const BW::vector<TriIndex>& faces ) const
{
	// Find the point with the greatest distance from any face
	float dist = -FLT_MAX;
	uint nextVert;

	// Loop through the verts
	BW::vector<uint>::const_iterator vert_it;
	for ( vert_it = verts.begin(); vert_it != verts.end(); vert_it++ )
	{
		// Loop through the faces, checking the distance of the vert against
		// each face.  If the distance is greater than the current max, set as
		// next vert
		BW::vector<TriIndex>::const_iterator face_it;
		for ( face_it = faces.begin(); face_it != faces.end(); face_it++ )
		{
			float faceDist = PlaneEq(	Vector3( vertices[face_it->i1].pos_ ),
										Vector3( vertices[face_it->i2].pos_ ),
										Vector3( vertices[face_it->i3].pos_ ))
								.distanceTo( Vector3( vertices[*vert_it].pos_ ) );
			if ( faceDist > dist )
			{
				nextVert = *vert_it;
				dist = faceDist;
			}
		}
	}

	return nextVert;
}


/**
 *	This method determines the edge horizon for vertIndex and expands the convex hull from
 *	the edge horizon to vertIndex.
 *
 *	@param vertIndex	The index of the vertex to add to the convex hull.
 *	@param vertices		The vertices to be enclosed in the convex hull.
 *	@param verts		The verts outside the convex hull.
 *	@param faces		The faces of the convex hull.
 */
void HullMesh::mcExpandHull(	const uint vertIndex, const VertexVector& vertices,
								BW::vector<uint>& verts, BW::vector<TriIndex>& faces ) const
{
	// Determine the front-facing and back-facing faces w.r.t. vertIndex
	BW::vector<uint> frontFacing;
	BW::vector<uint> backFacing;
	for ( size_t i = 0; i < faces.size(); i++ )
	{
		if ( PlaneEq(	Vector3( vertices[faces[i].i1].pos_ ),
						Vector3( vertices[faces[i].i2].pos_ ),
						Vector3( vertices[faces[i].i3].pos_ ))
				.isInFrontOfExact( Vector3( vertices[vertIndex].pos_ ) ) )
		{
			// add to front-facing list
			frontFacing.push_back(static_cast<uint>(i));
		}
		else
		{
			// Add to back-facing list
			backFacing.push_back(static_cast<uint>(i));
		}
	}

	// Find horizon vertices
	BW::vector<uint> horizonVerts;

	// Cover the special case of a single front facing triangle
	if ( frontFacing.size() == 1 )
	{
		horizonVerts.push_back( faces[frontFacing[0]].i1 );
		horizonVerts.push_back( faces[frontFacing[0]].i2 );
		horizonVerts.push_back( faces[frontFacing[0]].i3 );
		faces.erase( faces.begin() + frontFacing[0], faces.begin() + frontFacing[0] + 1 );
		this->mcRemoveVertex( vertIndex, vertices, verts );
		this->mcAddFaces( vertIndex, horizonVerts, faces );
		return;
	}

	// We need to be able to handle the general case of many front facing triangle,
	// some of which do not touch the edge horizon, and others that touch the edge
	// horizon with one or more vertices

	// Any vertices that appears in both the front facing and back facing lists must lie on the
	// horizon.
	for ( size_t i = 0; i < frontFacing.size(); i++ )
	{
		for ( size_t j = 0; j < backFacing.size(); j++ )
		{
			BW::vector<uint>::iterator result;

			for ( int k = 0; k < 3; k++ )
			{
				if (faces[frontFacing[i]][k] == faces[backFacing[j]].i1 ||
					faces[frontFacing[i]][k] == faces[backFacing[j]].i2 ||
					faces[frontFacing[i]][k] == faces[backFacing[j]].i3)
				{
					result = std::find( horizonVerts.begin(), horizonVerts.end(), faces[frontFacing[i]][k]);
					if ( result == horizonVerts.end() )
						horizonVerts.push_back( faces[frontFacing[i]][k] );
				}
			}
		}
	}

	// Order the vertices that lie on the horizon in a clockwise order when viewed from the
	// vertIndex's position

	// Find a front facing triangle with two or more vertices on the edge horizon and with
	// a unique vert that no other front facing triangle is indexing
	BW::vector<uint> sortedHorizonVerts;
	size_t index;
	int count = 0;
	bool indexFound[] = { false, false, false };
	for ( size_t i = 0; i < frontFacing.size(); i++ )
	{
		indexFound[0] = false; indexFound[1] = false; indexFound[2] = false;
		count = 0;

		// Need to test each horizon vertex against each face vertex
		for ( size_t j = 0; j < horizonVerts.size(); j++ )
		{
			for ( int k = 0; k < 3; k++ )
			{
				if (horizonVerts[j] == faces[frontFacing[i]][k])
				{
					count++;
					indexFound[k] = true;
				}
			}
		}

		if ( count == 2 )
		{
			index = i;
			break;
		}

		if ( count == 3 )
		{
			indexFound[0] = false; indexFound[1] = false; indexFound[2] = false;

			// For each vertex of i
			for ( int j = 0; j < 3; j++ )
			{
				// For each triangle in frontFacing
				for ( size_t k = 0; k < frontFacing.size(); k++ )
				{
					// Don't check against self
					if ( k == i )
						continue;

					if (faces[frontFacing[i]][j] == faces[frontFacing[k]].i1 ||
						faces[frontFacing[i]][j] == faces[frontFacing[k]].i2 ||
						faces[frontFacing[i]][j] == faces[frontFacing[k]].i3)
					{
						indexFound[j] = true;
						break;
					}
				}
			}

			if ( !indexFound[0] || !indexFound[1] || !indexFound[2] )
			{
				index = i;
				break;
			}
		}
	}
	// This assert ensures that at least two of the face vertices are on the horizon
	MF_ASSERT ( count > 1 && "HullMesh::mcExpandHull - Failed to find triangle on horizon edge")

	// Add the verts to the sortedHorizonVerts in acsending order
	for ( int i = 0; i < 3; i++ )
	{
		if (!indexFound[i])
		{
			if (count == 2)
			{
				sortedHorizonVerts.push_back(faces[frontFacing[index]][i+1]);
				sortedHorizonVerts.push_back(faces[frontFacing[index]][i+2]);
				break;
			}
			else
			{
				sortedHorizonVerts.push_back(faces[frontFacing[index]][i+2]);
				sortedHorizonVerts.push_back(faces[frontFacing[index]][i]);
				sortedHorizonVerts.push_back(faces[frontFacing[index]][i+1]);
				break;
			}
		}
	}

	// Find the front facing triangles with two (or more) verts on the horizon, one of which is
	// sortedHorizonVerts.back() and the next that is not already in sortedHorizonVerts.  Also
	// Make sure this edge is and external edge of the front facing triangles

	// While there are more vertices to account for
	while ( sortedHorizonVerts.size() != horizonVerts.size() )
	{
		bool found = false;

		// Loop through the front faces
		for ( size_t j = 0; j < frontFacing.size(); j++ )
		{
			// Loop through the vertices of the jth front face
			for ( int k = 0; k < 3; k++ )
			{
				// If the kth vert is the last vert in sortedHorizonVerts
				if (faces[frontFacing[j]][k] == sortedHorizonVerts.back())
				{
					bool inSortedList = false;

					// Cycle through sorted verts
					for ( size_t l = 0; l < sortedHorizonVerts.size(); l++ )
					{
						// Check if equal
						if (faces[frontFacing[j]][k+1] == sortedHorizonVerts[l])
						{
							inSortedList = true;
							break;
						}
					}

					if (!inSortedList)
					{
						// Check if connecting edge in internal
						if ( !this->mcIsInternalEdge( faces, frontFacing, sortedHorizonVerts.back(), faces[frontFacing[j]][k+1]) )
						{
							sortedHorizonVerts.push_back(faces[frontFacing[j]][k+1]);
							found = true;
							break;
						}
					}
				}
			}

			if (found)
				break;
		}

		if (!found)
			break;
	}

	// Delete all the front faces from faces in reverse order
	std::sort(frontFacing.begin(), frontFacing.end());
	std::reverse(frontFacing.begin(), frontFacing.end());
	for ( size_t i = 0; i < frontFacing.size(); i++ )
	{
		faces.erase( faces.begin() + frontFacing[i], faces.begin() + frontFacing[i] + 1 );
	}

	// Add the new faces
	this->mcAddFaces( vertIndex, sortedHorizonVerts, faces );

	// Delete the vertIndex
	this->mcRemoveVertex( vertIndex, vertices, verts );
}


bool HullMesh::mcIsInternalEdge(BW::vector<TriIndex>& faces, const BW::vector<uint>& frontFacing,
								const uint vertIndex1, const uint vertIndex2) const
{
	// Check if the edge is used by a single front facing triangle
	int edgeUseCount = 0;
	for ( size_t i = 0; i < frontFacing.size(); i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			if (	(vertIndex1 == faces[frontFacing[i]][j] && vertIndex2 == faces[frontFacing[i]][j+1]) ||
					(vertIndex2 == faces[frontFacing[i]][j] && vertIndex1 == faces[frontFacing[i]][j+1]) )
				edgeUseCount++;
		}
	}

	if (edgeUseCount > 1)
		return true;
	else
		return false;
}


/**
 *	This method adds a triangle fan with base vertIndex around the closed loop
 *	of vert defined by horizonVerts.
 *
 *	@param vertIndex	The index of the vertex being added to the convex hull.
 *	@param horizonVerts	The list of vertices on the edge horizon.
 *	@param faces		The faces of the convex hull.
 */
void HullMesh::mcAddFaces(	const uint vertIndex, const BW::vector<uint>& horizonVerts,
							BW::vector<TriIndex>& faces ) const
{
	for ( size_t i = 0; i < horizonVerts.size(); i++ )
	{
		faces.push_back( TriIndex( horizonVerts[i], horizonVerts[(i + 1) % horizonVerts.size()], vertIndex ) );
	}
}

BW_END_NAMESPACE

