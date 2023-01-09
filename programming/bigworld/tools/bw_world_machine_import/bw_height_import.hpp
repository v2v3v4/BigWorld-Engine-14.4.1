#ifndef BIGWORLD_HEIGHT_IMPORTER_HPP
#define BIGWORLD_HEIGHT_IMPORTER_HPP

// Where the base class generator is derived from
#include "core/IODevices.h"

#include "space_helper.hpp"

/**
 *	This class is a generator that imports heights from a BigWorld space
 */
class BWHeightImport :
	public Generator
{
public:

	static const char* HEIGH_SCALE_PROPERTY;
	static const char* CHUNK_SCALE_PROPERTY;

	BWHeightImport(void);
	virtual ~BWHeightImport(void);

	virtual bool Load(std::istream &in);
	virtual bool Save(std::ostream &out);

	virtual char *GetDescription() { return "Import height map from BigWorld space";};
	virtual char *GetTypeName() { return "BigWorld Height Importer"; };

	// The hasOrigin() property is false because this generator doesn't have an origin.
	virtual bool hasOrigin() { return false; };

	virtual bool Activate(BuildContext &context);

	void resetChunkScale();

	void selectSpace();


protected:
	BW::SpaceHelper spaceHelper_;
};

#endif // BIGWORLD_HEIGHT_IMPORTER_HPP
