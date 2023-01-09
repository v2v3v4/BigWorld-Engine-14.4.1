#ifndef __constraints_hpp__
#define __constraints_hpp__

#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

class Constraints
{
public:
	static void saveConstraints( DataSectionPtr pVisualSection );

protected:

	static void writeOrientConstraint( const MObject& object, 
		DataSectionPtr pConstraintsSection  );
	static void writePointConstraint( const MObject& object, 
		DataSectionPtr pConstraintsSection );
	static void writeParentConstraint( const MObject& object, 
		DataSectionPtr pConstraintsSection );
	static void writeAimConstraint( const MObject& object, 
		DataSectionPtr pConstraintsSection );
	static void writeIKHandle( const MObject& object, 
		DataSectionPtr pConstraintsSection );
};

BW_END_NAMESPACE

#endif // __constraints_hpp__
