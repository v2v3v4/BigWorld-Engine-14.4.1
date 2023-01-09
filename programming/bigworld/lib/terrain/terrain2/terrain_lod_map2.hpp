#ifndef TERRAIN_LOD_MAP2_HPP
#define TERRAIN_LOD_MAP2_HPP

BW_BEGIN_NAMESPACE

namespace Terrain
{

/**
 *	This class implements the lod texture for the terrain version 2
 */
class TerrainLodMap2 : public SafeReferenceCount
{
public:

	TerrainLodMap2();
	~TerrainLodMap2();

	/**
	 *  Load lod map from a section
	 */
	bool load(const DataSectionPtr& pLodMapSection );
	bool save(const DataSectionPtr& parentSection, const BW::string& name ) const;

	/**
	 *  Get the lod map
	 */
	const ComObjectWrap<DX::Texture>& pTexture() const { return pLodMap_; }
	void pTexture(const ComObjectWrap<DX::Texture>& texture ) { pLodMap_ = texture; } 

	/**
	 * Get the lod map size
	 */
	uint32 size() const { return pLodMap_ ? size_ : 0; }
private:

	ComObjectWrap<DX::Texture>  pLodMap_;
	uint32 size_;
};

typedef SmartPointer<TerrainLodMap2> TerrainLodMap2Ptr;

}

BW_END_NAMESPACE

#endif // TERRAIN_LOD_MAP2_HPP
