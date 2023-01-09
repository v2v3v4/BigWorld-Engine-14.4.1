#ifndef TERRAIN_GRAPHICS_OPTIONS_HPP
#define TERRAIN_GRAPHICS_OPTIONS_HPP

#include "resmgr/datasection.hpp"
#include "cstdmf/concurrency.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{

class TerrainSettings;

/**
 *  This class implements the terrain graphics options.
 *	The graphics options apply to the terrain settings.
 *	Currently the terrain graphics options let you modify
 *	the lod distances and also the top lod level.
 */
class TerrainGraphicsOptions
{
public:
	TerrainGraphicsOptions();

	static TerrainGraphicsOptions* instance();

	void init( DataSectionPtr pTerrain2Defaults );

	void addSettings( TerrainSettings* pSettings );
	void delSettings( TerrainSettings* pSettings );

	float lodModifier() const			{ MF_ASSERT(currentOption_); return currentOption_->m_vertexLodScale; }
	float lodTextureModifier() const	{ MF_ASSERT(currentOption_); return currentOption_->m_textureLodScale; }
	float lodNormalModifier() const		{ MF_ASSERT(currentOption_); return currentOption_->m_normalLodScale; }
	float lodBumpModifier() const		{ MF_ASSERT(currentOption_); return currentOption_->m_bumpLodScale; }
	bool  bumpMapping() const			{ MF_ASSERT(currentOption_); return currentOption_->m_useBumpMapping; }

private:
	void initOptions( DataSectionPtr pOptions );
	void setQualityOption(int optionIdx);

	//-- Represents parameters set of each individual graphics option.
	struct GraphicsOption
	{
		GraphicsOption(bool useBumpMapping, uint meshLoadBias, float vertexLodScale,
			float lodTextureScale, float lodNormalScale, float lodBumpScale
			)	:	m_useBumpMapping(useBumpMapping), m_meshLodBias(meshLoadBias), m_vertexLodScale(vertexLodScale),
					m_textureLodScale(lodTextureScale), m_normalLodScale(lodNormalScale), m_bumpLodScale(lodBumpScale) { }

		bool  m_useBumpMapping;
		uint  m_meshLodBias;	 //-- mesh resolution	
		float m_vertexLodScale;	 //-- lod transition scale
		float m_textureLodScale; //-- lod-map scale
		float m_normalLodScale;  //-- normal-map scale
		float m_bumpLodScale;    //-- bump-map scale
	};

	SmartPointer<Moo::GraphicsSetting>	pQualitySettings_;
	BW::vector<GraphicsOption>			graphicsOptions_;
	BW::vector< TerrainSettings* > settings_;
	mutable GraphicsOption*				currentOption_;
	SimpleMutex	mutex_;
};

} // namespace Terrain

BW_END_NAMESPACE

#endif //  TERRAIN_GRAPHICS_OPTIONS_HPP
