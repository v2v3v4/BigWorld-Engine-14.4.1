#ifndef MATERIAL_KINDS_HPP
#define MATERIAL_KINDS_HPP

#include "cstdmf/init_singleton.hpp"
#include "cstdmf/stringmap.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This singleton class handles loading and parsing the material
 *	kinds xml file, and provides accessors to the information
 *	contained therein.
 */
class MaterialKinds : public InitSingleton<MaterialKinds>
{
public:
	MaterialKinds();
	virtual ~MaterialKinds() {}

	typedef BW::map< uint32, DataSectionPtr > Map;

	//retrieve material kind ID by texture Name
	uint32 get( const BW::StringRef & textureName, bool preformatted = false );

	//retrieve a string associated with a material kind
	BW::string userString( uint32 materialKind,
		const BW::StringRef & keyName );

	//retrieve all data associated with a material kind
	DataSectionPtr userData( uint32 materialKind );

	//retrieve data associated with a material kind's texture.
	DataSectionPtr textureData( uint32 materialKind,
		const BW::StringRef & textureName );

	//indicate whether the provided material kind is valid
	bool isValid( uint32 materialKind );

	//the list of material kinds
	const MaterialKinds::Map& materialKinds() const
	{
		return materialKinds_;
	}

#ifdef EDITOR_ENABLED
	///Populate a data section with ID, Description pairs
	void populateDataSection( DataSectionPtr pSection );
	///Copy material kinds to a string map
	void createDescriptionMap( StringHashMap<uint32>& retMap );
	///Reload material kinds
	void reload();
#endif	//EDITOR_ENABLED


protected:
	bool doInit();
	bool doFini();


private:

	static BW::StringRef format( const StringRef & textureName, 
		char * ptr, size_t size );

	void addMaterialKind( DataSectionPtr pSection );
	StringRefUnorderedMap<uint32> textureToID_;
	MaterialKinds::Map materialKinds_;

	typedef MaterialKinds This;
};

BW_END_NAMESPACE

#endif
