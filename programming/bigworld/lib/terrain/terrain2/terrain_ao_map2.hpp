#ifndef TERRAIN_AO_MAP2_HPP
#define TERRAIN_AO_MAP2_HPP

#include "../terrain_data.hpp"
#include "terrain_renderer2.hpp"
#include "moo/image.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{

/**
 *	This class implements the ambient occlusion (AO) map resource for the terrain block 2
 */
class TerrainAOMap2 : public SafeReferenceCount
{
public:
	TerrainAOMap2();
	~TerrainAOMap2();

	bool init(const BW::string& terrainResource);

#ifdef EDITOR_ENABLED
	bool save(const DataSectionPtr& pTerrainSection);
	void regenerate(uint size);
	bool import(const Moo::Image<uint8>& srcImage, uint top, uint left, uint size);
#endif

	DX::Texture*	texture() const	{ return pAOMap_.pComObject(); }
	uint32			size() const	{ return pAOMap_.hasComObject() ? size_ : 0; }

private:
	static ComObjectWrap<DX::Texture> loadMap(const DataSectionPtr& pMapSection, uint32& size );

#ifdef EDITOR_ENABLED
	static bool createMap(const Moo::Image<uint8>& srcImg, ComObjectWrap<DX::Texture>& pMap);
	static bool saveMap(const DataSectionPtr& pSection, const ComObjectWrap<DX::Texture>& pMap);
#endif

	ComObjectWrap<DX::Texture>  pAOMap_;
	uint32					    size_;
	BW::string					terrainResource_;
};

typedef SmartPointer<TerrainAOMap2> TerrainAOMap2Ptr;

}

BW_END_NAMESPACE

#endif // TERRAIN_AO_MAP2_HPP