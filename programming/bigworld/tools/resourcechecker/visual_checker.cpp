#include "pch.hpp"

#include "visual_checker.hpp"

#include "math/planeeq.hpp"
#include "math/boundbox.hpp"

#include "resmgr/dataresource.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/primitive_file.hpp"
#include "moo/primitive_file_structs.hpp"
#include "moo/vertex_formats.hpp"

#include "cstdmf/log_msg.hpp"

#include "shlwapi.h"
#pragma comment( lib, "Shlwapi.lib" )

DECLARE_DEBUG_COMPONENT2( "Checker", 0 )

BW_BEGIN_NAMESPACE

namespace
{


	BW::string toLower( const BW::string &s )
	{
		BW::string newString = s;

		for( uint32 i = 0; i < newString.length(); i++ )
		{
			if( newString[ i ] >= 'A' && newString[ i ] <= 'Z' )
			{
				newString[ i ] = newString[ i ] + 'a' - 'A';
			}
		}
		return newString;
	}

	bool startsWith( const BW::string& s, const BW::string& prefix )
	{
		if (s.length() < prefix.length())
			return false;

		return (s.substr(0, prefix.length()) == prefix);
	}

	bool endsWith( const BW::string& s, const BW::string& postfix )
	{
		if (s.length() < postfix.length())
			return false;

		return (s.substr(s.length() - postfix.length()) == postfix);
	}


	/**
	 * Sorts rule DataSections by length of directiory name
	 *
	 * If dir name is equal, sort by file spec
	 */
	bool ruleSectionCompare(const DataSectionPtr a, DataSectionPtr b) 
	{
		size_t aPathSize = a->readString( "path" ).size(); 
		size_t bPathSize = b->readString( "path" ).size();

		if (aPathSize == bPathSize)
			return a->readString( "filespec" ).size() > b->readString( "filespec" ).size();
		else
			return aPathSize > bPathSize;
	}

	Vector3 recursiveReadVector3(const BW::string& key,
		const BW::map<BW::string, DataSectionPtr>& sections,
		DataSectionPtr curSection,
		Vector3 def)
	{
		DataSectionPtr ds = curSection->openSection( key );
		if (ds)
			return ds->asVector3();

		BW::string parent = curSection->readString( "parent" );

		BW::map<BW::string, DataSectionPtr>::const_iterator i = sections.find( parent );
		if (i != sections.end())
			return recursiveReadVector3( key, sections, i->second, def );
		
		return def;
	}

	BW::string recursiveReadString(const BW::string& key,
		const BW::map<BW::string, DataSectionPtr>& sections,
		DataSectionPtr curSection,
		const BW::string& def)
	{
		DataSectionPtr ds = curSection->openSection( key );
		if (ds)
			return ds->asString();

		BW::string parent = curSection->readString( "parent" );

		BW::map<BW::string, DataSectionPtr>::const_iterator i = sections.find( parent );
		if (i != sections.end())
			return recursiveReadString( key, sections, i->second, def );
		
		return def;
	}

	int recursiveReadInt(const BW::string& key,
		const BW::map<BW::string, DataSectionPtr>& sections,
		DataSectionPtr curSection,
		int def)
	{
		DataSectionPtr ds = curSection->openSection( key );
		if (ds)
			return ds->asInt();

		BW::string parent = curSection->readString( "parent" );

		BW::map<BW::string, DataSectionPtr>::const_iterator i = sections.find( parent );
		if (i != sections.end())
			return recursiveReadInt( key, sections, i->second, def );
		
		return def;
	}

	float recursiveReadFloat(const BW::string& key,
		const BW::map<BW::string, DataSectionPtr>& sections,
		DataSectionPtr curSection,
		float def)
	{
		DataSectionPtr ds = curSection->openSection( key );
		if (ds)
			return ds->asFloat();

		BW::string parent = curSection->readString( "parent" );

		BW::map<BW::string, DataSectionPtr>::const_iterator i = sections.find( parent );
		if (i != sections.end())
			return recursiveReadFloat( key, sections, i->second, def );
		
		return def;
	}

	bool recursiveReadBool(const BW::string& key,
		const BW::map<BW::string, DataSectionPtr>& sections,
		DataSectionPtr curSection,
		bool def)
	{
		DataSectionPtr ds = curSection->openSection( key );
		if (ds)
			return ds->asBool();

		BW::string parent = curSection->readString( "parent" );

		BW::map<BW::string, DataSectionPtr>::const_iterator i = sections.find( parent );
		if (i != sections.end())
			return recursiveReadBool( key, sections, i->second, def );
		
		return def;
	}

	void recursiveReadStrings(const BW::string& key,
		const BW::map<BW::string, DataSectionPtr>& sections,
		DataSectionPtr curSection,
		BW::set<BW::string>& results)
	{

		BW::vector<BW::string> strs;
		curSection->readStrings( key, strs );
		results.insert( strs.begin(), strs.end() );

		BW::string parent = curSection->readString( "parent" );

		BW::map<BW::string, DataSectionPtr>::const_iterator i = sections.find( parent );
		if (i != sections.end())
			recursiveReadStrings( key, sections, i->second, results );
	}

}

// -----------------------------------------------------------------------------
// Section: VisualChecker
// -----------------------------------------------------------------------------

VisualChecker::VisualChecker( const BW::string& visualName, bool cacheRules, bool snapVertices )
	: minSize_( Vector3::zero() )
	, maxSize_( Vector3::zero() )
	, minTriangles_( 0 )
	, maxTriangles_( 0 )
	, maxHierarchyDepth_( 0 )
	, snapVertices_(snapVertices)
	, portals_( false )
	, portalSnap_( Vector3::zero() )
	, portalDistance_( 0.f )
	, portalOffset_( 0.f )
	, checkHardPoints_( false )
	, typeName_( UNKNOWN_TYPE_NAME )
	, exportAs_( "normal" )
{
	// Do no checking for collision visuals
	if (visualName.find( "_bsp." ) != BW::string::npos)
	{
		return;
	}

	DataSectionPtr rulesSection;
	if (cacheRules)
	{
		rulesSection = BWResource::openSection( "visual_rules.xml" );
	}
	else
	{
		if (BWResource::fileExists( "visual_rules.xml" ) )
			rulesSection = BWResource::openSection( "visual_rules.xml" );
	}

	// No rules, allow everything
	if (!rulesSection)
	{
		if ( ! LogMsg::automatedTest() )
		{
			MessageBox( GetForegroundWindow(),
				TEXT("VisualChecker::VisualChecker - Unable to find visual_rules.xml file.\n")
				TEXT("VisualChecker will be disabled temporarily\n"),
				TEXT("Visual Exporter"), MB_OK | MB_ICONEXCLAMATION );
		}
		addError("VisualChecker::VisualChecker - Unable to find visual_rules.xml file.");
		return;
	}

	// Get all matching rules, sorted by directory length
	BW::string visualPath = BWResource::getFilePath( visualName );
	BW::string visualFilename = BWResource::getFilename( visualName ).to_string();

	BW::vector<DataSectionPtr> matchingRules;
	BW::vector<DataSectionPtr> rules;
	rulesSection->openSections( "rule", rules );
	for (BW::vector<DataSectionPtr>::iterator i = rules.begin(); i != rules.end(); ++i)
	{
		BW::string rulePath = toLower( (*i)->readString( "path" ) );

		if ( visualPath.find( rulePath ) != BW::string::npos )
			matchingRules.push_back( *i );
	}

	std::stable_sort( matchingRules.begin(), matchingRules.end(), ruleSectionCompare );

	// Traverse down the list until we find a file pattern that matches
	DataSectionPtr ruleSection;
	for (BW::vector<DataSectionPtr>::iterator i = matchingRules.begin(); i != matchingRules.end(); ++i)
	{
		BW::string filePattern = toLower( (*i)->readString( "filespec" ) );

		// Matches everything, we've found something
		if (filePattern.empty())
		{
			ruleSection = (*i);
			break;
		}

#ifdef UNICODE
		if ( ::PathMatchSpec( bw_utf8tow( visualFilename ).c_str(), bw_utf8tow( filePattern ).c_str() ) )
#else
		if ( ::PathMatchSpec( visualFilename.c_str(), filePattern.c_str() ) )
#endif
		{
			ruleSection = (*i);
			break;
		}
	}

	// No matching rule section, bail
	if (!ruleSection)
		return;

	// Build a map of rule id -> rule section
	BW::map<BW::string, DataSectionPtr> rulesById;
	for (BW::vector<DataSectionPtr>::iterator i = rules.begin(); i != rules.end(); ++i)
	{
		BW::string id = (*i)->readString( "identifier" );
		if (!id.empty())
			rulesById[id] = *i;
	}

	typeName_ = ruleSection->readString( "identifier" );
	exportAs_ = recursiveReadString( "exportAs", rulesById, ruleSection, exportAs_ );
	if (!(exportAs_ == "normal" || exportAs_ == "static" || exportAs_ == "static with nodes"))
	{
		char s[1024];
		bw_snprintf( s, sizeof(s), "Unknown value for exportAs [%s]\n", exportAs_.c_str() );
		if ( ! LogMsg::automatedTest() )
		{
#ifdef UNICODE
			MessageBox( GetForegroundWindow(), bw_utf8tow( s ).c_str(), TEXT("Visual Exporter"), 
						MB_OK | MB_ICONEXCLAMATION );
#else
			MessageBox( GetForegroundWindow(), s, TEXT("Visual Exporter"), MB_OK | MB_ICONEXCLAMATION );
#endif
		}
		addError(s);
		exportAs_ = "normal";
	}

	minSize_ = recursiveReadVector3( "minSize", rulesById, ruleSection, minSize_ );
	maxSize_ = recursiveReadVector3( "maxSize", rulesById, ruleSection, maxSize_ );
	minTriangles_ = recursiveReadInt( "minTriangles", rulesById, ruleSection, minTriangles_ );
	maxTriangles_ = recursiveReadInt( "maxTriangles", rulesById, ruleSection, maxTriangles_ );
	maxHierarchyDepth_ = recursiveReadInt( "maxHierarchyDepth", rulesById, ruleSection, maxHierarchyDepth_ );
	portals_ = recursiveReadBool( "portals", rulesById, ruleSection, portals_ );
	portalSnap_ = recursiveReadVector3( "portalSnap", rulesById, ruleSection, portalSnap_ );
	portalDistance_ = recursiveReadFloat( "portalDistance", rulesById, ruleSection, portalDistance_ );
	portalOffset_ = recursiveReadFloat( "portalOffset", rulesById, ruleSection, portalOffset_ );
	checkHardPoints_ = recursiveReadBool( "checkUnknownHardPoints", rulesById, ruleSection, checkHardPoints_ );

	recursiveReadStrings( "hardPoint", rulesById, ruleSection, hardPoints_ );
}

namespace 
{
	float snapValue( float& v, float snapSize )
	{
		if ( snapSize > 0.f )
		{
			float halfSnap = snapSize / 2.f;

			if (v > 0.f)
			{
				v += halfSnap;

				v -= ( fmodf( v, snapSize ) );
			}
			else
			{
				v -= halfSnap;

				v += ( fmodf( -v, snapSize ) ); 
			}
		}

		return v;
	}

	Vector3 snapVector3( const Vector3& vec, const Vector3& snaps )
	{
		Vector3 v = vec;

		if (snaps.x != 0.f)
			snapValue( v.x, snaps.x );
		if (snaps.y != 0.f)
			snapValue( v.y, snaps.y );
		if (snaps.z != 0.f)
			snapValue( v.z, snaps.z );

		return v;
	}

	bool isMultipleOf( float v, float m, const float epsilon = (1.0f / 8192.f) )
	{
		float diff = fabsf( fmodf( v, m ) );

		return (diff < epsilon || diff - m > -epsilon);
	}

	bool close( float a, float b )
	{
		return fabsf(a - b) < (1.0f / 8192.f);
	}

	/**
	 *	Recursively check all node names for duplicates.
	 */
	void checkDuplicateNodeNames( DataSectionPtr nodeSection, BW::set<BW::string>& nodeNames, BW::set<BW::string>& duplicates )
	{
		if (!nodeSection)
			return;

		BW::string id = nodeSection->readString( "identifier" );
		if (std::find( nodeNames.begin(), nodeNames.end(), id ) == nodeNames.end())
		{
			nodeNames.insert( id );
		}
		else
		{
			duplicates.insert( id );
		}

		BW::vector<DataSectionPtr> nodes;
		nodeSection->openSections( "node", nodes );
		for (BW::vector<DataSectionPtr>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			checkDuplicateNodeNames( *i, nodeNames, duplicates );
		}
	}

	/**
	 * Recursivly get all the hardpoints under nodeSection
	 */
	void extractHardPoints( DataSectionPtr nodeSection, BW::set<BW::string>& hardPoints )
	{
		if (!nodeSection)
			return;

		BW::string id = nodeSection->readString( "identifier" );
		if (id.length() > 3 && id.substr( 0, 3 ) == "HP_")
		{
			hardPoints.insert( id );
		}

		BW::vector<DataSectionPtr> nodes;
		nodeSection->openSections( "node", nodes );
		for (BW::vector<DataSectionPtr>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			extractHardPoints( *i, hardPoints );
		}
	}

	/**
	 *	Traverse nodes to find the maximum depth of the node tree
	 */
	uint32 maxNodeDepth( DataSectionPtr nodeSection )
	{
		uint32 maxDepth = 0;

		BW::vector<DataSectionPtr> nodes;
		nodeSection->openSections( "node", nodes );
		for (BW::vector<DataSectionPtr>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			maxDepth = max( maxDepth, maxNodeDepth( *i ) );
		}

		return maxDepth + 1;
	}	
}

bool VisualChecker::check( DataSectionPtr visualSection, const BW::string& primResName )
{
	errors_.clear();

	bool good = true;

	if (!visualSection)
	{
		addError( "Invalid visual, null DataSection" );
		return false;
	}

	DataSectionPtr spPrims = PrimitiveFile::get( primResName );
	if (!spPrims)
	{
		addError( "Invalid primitive file, file not found" );
		return false;
	}

	Vector3 bbMin = visualSection->readVector3( "boundingBox/min" );
	Vector3 bbMax = visualSection->readVector3( "boundingBox/max" );
	Vector3 size = bbMax - bbMin;

	// Check it's bigger than min size
	if ((minSize_.x > 0.f && minSize_.x > size.x) ||
		(minSize_.y > 0.f && minSize_.y > size.y) ||
		(minSize_.z > 0.f && minSize_.z > size.z))
	{
		good = false;
		addError( "Too small, must be at least %f, %f, %f", minSize_.x, minSize_.y, minSize_.z );
	}

	// Check it's smaller than max size
	if ((maxSize_.x > 0.f && maxSize_.x < size.x) ||
		(maxSize_.y > 0.f && maxSize_.y < size.y) ||
		(maxSize_.z > 0.f && maxSize_.z < size.z))
	{
		good = false;
		addError( "Too big, must be no bigger than %f, %f, %f", maxSize_.x, maxSize_.y, maxSize_.z );
	}

	// Check that it doesn't have too many triangles
	if ( !checkTriangleCount( spPrims ) )
	{
		//error added in checkTriangleCount method.
		good = false;
	}

	uint32 hierarchyDepth = maxNodeDepth( visualSection->openSection( "node" ) );

	if (maxHierarchyDepth_ != 0 &&
		hierarchyDepth > maxHierarchyDepth_ )
	{
		good = false;
		addError( "Node hierarchy too deep, depth must be %d nodes or less", maxHierarchyDepth_ );
	}


	// Check that its portals are correct
	BW::vector<DataSectionPtr> boundaries;
	visualSection->openSections( "boundary", boundaries );
	BW::vector< std::pair<DataSectionPtr,DataSectionPtr> > portals;
	for ( uint32 i=0; i<boundaries.size(); i++ )
	{
		BW::vector<DataSectionPtr> portalSections;
		boundaries[i]->openSections( "portal", portalSections );
		for ( uint32 j=0; j<portalSections.size(); j++ )
		{
			portals.push_back( std::make_pair(boundaries[i],portalSections[j]) );
		}
	}

	if (portals_ && portals.empty())
	{
		good = false;
		addError( "Must have one or more portals" );
	}
	if (!portals_ && !portals.empty())
	{
		good = false;
		addError( "Must not have any portals" );
	}

	if (portals_)
	{
		BW::vector< std::pair<DataSectionPtr,DataSectionPtr> >::iterator i;
		for (i = portals.begin(); i != portals.end(); ++i)
		{		
			DataSectionPtr boundary = i->first;
			DataSectionPtr portal = i->second;

			// Check that the portal is alright
			BW::vector<Vector3> points;
			portal->readVector3s( "point", points );

			// Portal points get mapped back into object space,
			// from plane space.
			Matrix basis;
			basis[0] = portal->readVector3( "uAxis" );
			basis[2] = boundary->readVector3( "normal" );
			basis[1] = basis[2].crossProduct( basis[0] );
			basis.translation( boundary->readVector3("normal") * boundary->readFloat("d") );
			for (uint32 j=0; j<points.size(); j++)
			{
				Vector3& pt = points[j];
				pt = basis.applyPoint(pt);
			}

			// Check the portal is valid
			if (points.size() < 3)
			{
				good = false;
				addError( "Not enough points in a portal" );
				continue;
			}

			Vector3 portalCentre(0.f, 0.f, 0.f);
			for (uint j = 0; j < points.size(); j++)
				portalCentre += points[j];
			portalCentre *= 1.f / points.size();

			PlaneEq p;
			bool hasPlaneEq = false;
			for (uint j = 0; j < (points.size() - 2); j++)
			{
				p.init( points[ j + 2 ], points[ j + 1 ], points[ j ] );

				if (!((p.normal()[0] == 0) && (p.normal()[1] == 0) && (p.normal()[2] == 0)))
				{
					hasPlaneEq = true;
					break;
				}
			}

			if (!hasPlaneEq)
			{
				good = false;
				addError( "Portal is non planar" );
				continue;
			}

			BoundingBox bb(bbMin, bbMax);
			if (p.d() - bb.centre().dotProduct(p.normal()) > 0.f)
			{
				good = false;
				addError( "Portal is facing outwards" );
				continue;
			}

			/*
			// Check it's on a boundary
			// 9/10/04 PJ - visual exporter can now only export portals on boundaries.
			if (!((close(p.normal().x, 1.f) && close(fabsf(p.d()), fabsf(bbMin.x))) ||
				(close(p.normal().x, -1.f) && close(fabsf(p.d()), fabsf(bbMax.x))) ||
				(close(p.normal().y, 1.f) && close(fabsf(p.d()), fabsf(bbMin.y))) ||
				(close(p.normal().y, -1.f) && close(fabsf(p.d()), fabsf(bbMax.y))) ||
				(close(p.normal().z, 1.f) && close(fabsf(p.d()), fabsf(bbMin.z))) ||
				(close(p.normal().z, -1.f) && close(fabsf(p.d()), fabsf(bbMax.z)))))			
			{
				good = false;
				addError( "Portal %d (%f, %f, %f, %f) not on bounding box",
					(i - portals.begin()) / sizeof(i),
					p.normal().x, p.normal().y, p.normal().z, p.d() );
				continue;
			}*/

			// Check it's snapped correctly
			BoundingBox portalBB = BoundingBox::s_insideOut_;
			for (uint i = 0; i < points.size(); ++i)
			{
				portalBB.addBounds( points[i] );
			}

			Vector3 minDiff = snapVector3( portalBB.minBounds(), portalSnap_ ) - portalBB.minBounds();
			Vector3 maxDiff = snapVector3( portalBB.maxBounds(), portalSnap_ ) - portalBB.maxBounds();

			static const float ERROR_EPSILON = 1.0f / 2196.f;
			if ((fabsf(minDiff.x) > ERROR_EPSILON) ||
				(fabsf(minDiff.y) > ERROR_EPSILON) ||
				(fabsf(minDiff.z) > ERROR_EPSILON) ||
				(fabsf(maxDiff.x) > ERROR_EPSILON) ||
				(fabsf(maxDiff.y) > ERROR_EPSILON) ||
				(fabsf(maxDiff.z) > ERROR_EPSILON))
			{
				good = false;
				addError( "Portals must reside on multiples of %f, %f, %f",
					portalSnap_.x, portalSnap_.y, portalSnap_.z );
			}

			// Check the portal is the correct distance from the origin
			if (portalDistance_ > 0.f)
			{
				if (!isMultipleOf( p.d(), portalDistance_, 1.0f / 1024.f ))
				{
					good = false;
					addError( "Portal must be a multiple of %f away from the origin (it's %f away)",
						portalDistance_,
						p.d() );
				}
			}

			// Check the portals offset from the origin on it's plane is good
			float offset = 0.f;
			if (close( fabsf( p.normal().x ), 1.f ))
			{
				offset = portalCentre.z;
			}

			if (close( fabsf( p.normal().z ), 1.f ))
			{
				offset = portalCentre.x;
			}

			if (offset != 0.f && portalOffset_ != 0.f)
			{
				if (!isMultipleOf( offset, portalOffset_ ))
				{
					good = false;
					addError( "Centre of portal must be a multiple of %f on the portals plane (it's %f away)",
						portalOffset_,
						offset );
				}
			}

		}
	}
	
	BW::set<BW::string> visualNodes;
	BW::set<BW::string> nodeDuplicates;

	checkDuplicateNodeNames( visualSection->openSection( "node" ), visualNodes, nodeDuplicates );
	for( BW::set<BW::string>::iterator it = nodeDuplicates.begin(); it != nodeDuplicates.end(); ++it )
	{
		good = false;
		addError( "Duplicate node name %s", (*it).c_str() );
	}

	// Check that its hard points are correct
	BW::set<BW::string> visualHardPoints;
	extractHardPoints( visualSection->openSection( "node" ), visualHardPoints );

	// Check we have all the hard points we need to
	for (BW::set<BW::string>::iterator i = hardPoints_.begin(); i != hardPoints_.end(); ++i)
	{
		if (!visualHardPoints.count(*i))
		{
			good = false;
			addError( "Missing hard point %s", (*i).c_str() );
		}
	}

	// Check we don't have any hard points we shouldn't
	if (checkHardPoints_)
	{
		for (BW::set<BW::string>::iterator i = visualHardPoints.begin(); i != visualHardPoints.end(); ++i)
		{
			if (!hardPoints_.count(*i))
			{
				// Check for a well formed flash HP
				// Must be along the lines of HP_flash01
				if ((*i).length() == 10 && (*i).substr(0, 8) == "HP_flash" && isdigit((*i)[8])
					&& isdigit((*i)[9]))
				{
					continue;
				}

				// HP_flash is also a valid flash hardpoint name
				if ((*i) == "HP_flash")
					continue;


				good = false;
				addError( "Unknown hard point %s", (*i).c_str() );
			}
		}
	}

	// Check that no texture name has a space in it
	typedef BW::vector<DataSectionPtr> DSVec;
	DSVec renderSets;
	visualSection->openSections( "renderSet", renderSets );
	for (uint32 rs = 0; rs < renderSets.size(); rs++)
	{
		DSVec geometries;
		renderSets[rs]->openSections( "geometry", geometries );
		for (uint32 g = 0; g < geometries.size(); g++)
		{
			DSVec pgs;
			geometries[g]->openSections( "primitiveGroup", pgs );
			for (uint32 pg = 0; pg < pgs.size(); pg++)
			{
				BW::string textureName = pgs[pg]->readString( "material/textureormfm" );
				BW::string::size_type pos = textureName.find_last_of( " " );
				if (pos != BW::string::npos)
				{
					good = false;
					addError( "Illegal texture name \"%s\", no spaces in name or path allowed\n", textureName.c_str() );
				}
			}
		}
	}

	return good;
}



namespace
{

typedef BW::map<BW::string, int> VertexSizes;
int vertexSize( const BW::string& format )
{
	static VertexSizes vs;
	if (vs.size() == 0)
	{
		vs["xyznuv"] = sizeof( Moo::VertexXYZNUV );
		vs["xyznduv"] = sizeof( Moo::VertexXYZNDUV );
		vs["xyznuvtb"] = sizeof( Moo::VertexXYZNUV );
		vs["xyznuvi"] = sizeof( Moo::VertexXYZNUVI );
		vs["xyznuvitb"] = sizeof( Moo::VertexXYZNUVITB );
		vs["xyznuviiiww"] = sizeof( Moo::VertexXYZNUVIIIWW );
		vs["xyznuviiiwwtb"] = sizeof( Moo::VertexXYZNUVIIIWWTB );
	}
	VertexSizes::iterator it = vs.find( format );
	if (it != vs.end())
	{
		return it->second;
	}

	return -1;
}

}

bool VisualChecker::checkTriangleCount( DataSectionPtr spPrims )
{
	uint32 count = 0;

	//Iterate through all sections and find the indices sections.
	DataSectionIterator it = spPrims->begin();
	while ( it != spPrims->end() )
	{
		DataSectionPtr pSection = *it++;

		if ( endsWith( pSection->sectionName(), "indices" ) )
		{
			Moo::IndexHeader* ih = (Moo::IndexHeader*)pSection->asBinary()->data();
			count += ih->nIndices_ / 3;
		}
	}

	if ( count > maxTriangles_ && maxTriangles_ != 0 )
	{
		addError( "Too many triangles (%d), must be less than %d", count, maxTriangles_ );
		return false;
	}

	if ( count < minTriangles_ )
	{
		addError( "Too few triangles (%d), must be at least %d", count, minTriangles_ );
		return false;
	}

	return true;
}


void VisualChecker::addError( const char * format, ... )
{
	va_list argPtr;
	va_start( argPtr, format );

	char buf[4096];
	_vsnprintf( buf, sizeof(buf), format, argPtr );
	buf[sizeof( buf ) - 1] = '\0';

	errors_.push_back( BW::string( buf ) );

	va_end(argPtr);
}

BW::string VisualChecker::errorText()
{
	BW::string s;
	BW::vector<BW::string>::iterator i = errors_.begin();
	for (; i != errors_.end(); ++i)
	{
		if (i != errors_.begin())
			s += '\n';

		s += *i;
	}

	return s;
}

BW_END_NAMESPACE

