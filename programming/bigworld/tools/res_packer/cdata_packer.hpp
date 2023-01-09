#ifndef __CDATA_PACKER_HPP__
#define __CDATA_PACKER_HPP__


#include "packers.hpp"
#include "cstdmf/stdmf.hpp"

#include <string>


BW_BEGIN_NAMESPACE

/**
 *	This class strips the .cdata files of unwanted data. In the client, it
 *	removes navmesh and thumbnail sections, and in the server only the 
 *	thumbnail section.
 */
class CDataPacker : public BasePacker
{
public:
	virtual bool prepare( const BW::string & src, const BW::string & dst );
	virtual bool print();
	virtual bool pack();

	static void addStripSection( const char * sectionName );

	CDataPacker();

private:
	DECLARE_PACKER()
	BW::string src_;
	BW::string dst_;
	bool stripWorldNavMesh_;
	bool stripLayers_;
	bool stripLodTextures_;
	bool stripQualityNormals_;
	bool stripLodNormals_;
	int32 heightMapLodToPreserve_;
	BW::string  terrainDataSectionName_;
};

BW_END_NAMESPACE

#endif // __CDATA_PACKER_HPP__
