#ifndef __portal_hpp__
#define __portal_hpp__

BW_BEGIN_NAMESPACE

class Portal
{
public:
	Portal();
	~Portal();

	// returns the number of portals
	uint32 count();

	// initialises Portal for portal "index" - access functions now use this
	// portal. this must be called before accessing vertex data
	bool initialise( uint32 index );

	// frees up memory used by the portal
	void finalise();

	MDagPathArray& portals();

	BW::string name();
	BW::string flags();
	BW::string label();
	BW::vector<Point3>& positions();

protected:
	// dag paths to portal
	MDagPathArray _portals;

	// geometry data
	BW::string _name;		// name of the portal mesh
	BW::string _flags;		// type of portal
	BW::string _label;		// label of portal
	BW::vector<Point3> _positions;
};

BW_END_NAMESPACE

#endif // __portal_hpp__
