#ifndef VISUAL_CHECKER_HPP
#define VISUAL_CHECKER_HPP

#include "resmgr/datasection.hpp"

#include <string>
#include "cstdmf/bw_set.hpp"

#define UNKNOWN_TYPE_NAME ( "(unknown)" )

BW_BEGIN_NAMESPACE

/**
 *	Ensures that a given visual passes a series of validity checks
 */
class VisualChecker
{
private:
	Vector3 minSize_;
	Vector3 maxSize_;
	uint32 maxTriangles_;
	uint32 minTriangles_;
	uint32 maxHierarchyDepth_;
	bool snapVertices_;
	bool portals_;
	Vector3 portalSnap_;
	float portalDistance_;
	float portalOffset_;
	bool checkHardPoints_;
	BW::set<BW::string> hardPoints_;

	BW::string typeName_;
	BW::string exportAs_;

	BW::vector<BW::string> errors_;

	bool checkTriangleCount( DataSectionPtr spPrims );
	void addError( const char * format, ... );
public:

	/** Construct a VisualChecker with setting appropriate for the given visual */
	VisualChecker( const BW::string& visualName, bool cacheRules = true, bool snapVertices = true );

	/** Check the visual for errors according to our current rules */
	bool check( DataSectionPtr visualSection, const BW::string& primResName );

	/** If check() failed, get the errors that occured, one per line */
	BW::string errorText();

	/** The type of visual we have determined from the file name */
	BW::string typeName() const { return typeName_; }

	/** The recommended type (static, static with nodes, normal) to export the visual as */
	BW::string exportAs() const { return exportAs_; }

	/** Sets the snap vertices flag. */
	void snapVertices( bool snapVertices ) { snapVertices_ = snapVertices; }
};

BW_END_NAMESPACE

#endif // VISUAL_CHECKER_HPP
