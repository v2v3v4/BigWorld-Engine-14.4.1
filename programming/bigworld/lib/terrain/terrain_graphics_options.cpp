#include "pch.hpp"
#include "terrain_graphics_options.hpp"
#include "manager.hpp"
#include "cstdmf/debug.hpp"
#include "terrain_settings.hpp"


BW_BEGIN_NAMESPACE

using namespace Terrain;


TerrainGraphicsOptions::TerrainGraphicsOptions() : currentOption_(NULL)	
{

}

/**
 *	This method inits the terrain graphics options
 */
void TerrainGraphicsOptions::init( DataSectionPtr pTerrain2Defaults )
{
	BW_GUARD;
	initOptions( pTerrain2Defaults->openSection( "lodOptions" ) );
}


/**
 *	Instance wrapper method for the TerrainGraphicsOptions
 */
TerrainGraphicsOptions* TerrainGraphicsOptions::instance()
{
	BW_GUARD;
	if (Manager::pInstance() == NULL)
		return NULL;

	return &Manager::instance().graphicsOptions();
}


/**
 *	This method initialises the lod options
 *	@param pOptions the options to use
 */
void TerrainGraphicsOptions::initOptions( DataSectionPtr pOptions )
{
	BW_GUARD;
	typedef Moo::GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;

	//--
	pQualitySettings_ = Moo::makeCallbackGraphicsSetting(
		"TERRAIN_QUALITY", "Terrain Quality", *this,
		&TerrainGraphicsOptions::setQualityOption, -1, true, false
		);

	//--
	pQualitySettings_->addOption("MAX",		"Max",		Moo::rc().psVersion() >= 0x300, true);
	pQualitySettings_->addOption("HIGH",	"High",		Moo::rc().psVersion() >= 0x300, true);
	pQualitySettings_->addOption("MEDIUM",	"Medium",	Moo::rc().psVersion() >= 0x300, true);
	pQualitySettings_->addOption("LOW",		"Low",		true);
	pQualitySettings_->addOption("MIN",		"Min",		true);

	//--
	graphicsOptions_.push_back(GraphicsOption(true,  0, 1.0f, 1.0f, 1.0f, 1.0f)); //-- MAX
	graphicsOptions_.push_back(GraphicsOption(true,  0, 1.0f, 0.8f, 0.8f, 0.6f)); //-- HIGH
	graphicsOptions_.push_back(GraphicsOption(true,  0, 1.0f, 0.6f, 0.6f, 0.2f)); //-- MEDIUM
	graphicsOptions_.push_back(GraphicsOption(false, 0, 1.0f, 0.4f, 0.4f, 1.0f)); //-- LOW
	graphicsOptions_.push_back(GraphicsOption(false, 1, 1.0f, 0.2f, 0.2f, 1.0f)); //-- MIN

	Moo::GraphicsSetting::add(pQualitySettings_);
	Moo::GraphicsSetting::commitPending();
}

/**
 *	This method is used by the graphics setting system.
 *	@param optionIdx the selected lod option
 */
void TerrainGraphicsOptions::setQualityOption(int optionIdx)
{
	BW_GUARD;
	MF_ASSERT((optionIdx >= 0) && (optionIdx < (int)graphicsOptions_.size()));
	SimpleMutexHolder sm(mutex_);

	currentOption_ = &graphicsOptions_[optionIdx];

	//-- 1. apply mesh resolution.
	{
		TerrainSettings::topVertexLod(currentOption_->m_meshLodBias);
	}

	//-- 2. apply bump mapping.
	{
		for (uint32 i = 0; i < settings_.size(); i++)
		{
			settings_[i]->bumpMapping(currentOption_->m_useBumpMapping);
			settings_[i]->applyBumpMappingModifier(currentOption_->m_bumpLodScale);
		}
	}

	//-- 3. apply lod modifiers.
	{
		for (uint32 i = 0; i < settings_.size(); i++)
		{
			settings_[i]->applyLodModifier(currentOption_->m_vertexLodScale);
			settings_[i]->applyLodTextureModifier(currentOption_->m_textureLodScale);
			settings_[i]->applyLodNormalModifier(currentOption_->m_normalLodScale);
		}
	}
}

/**
 *	This method adds a terrain settings object so that we can apply
 *	settings to it.
 */
void TerrainGraphicsOptions::addSettings( TerrainSettings* pSettings )
{
	BW_GUARD;
	SimpleMutexHolder sm(mutex_);
	settings_.push_back( pSettings );
}

/**
 *	This method removes a terrain settings object from the internal list.
 */
void TerrainGraphicsOptions::delSettings( TerrainSettings* pSettings )
{
	BW_GUARD;
	SimpleMutexHolder sm(mutex_);

	BW::vector< TerrainSettings* >::iterator it =
		std::find(settings_.begin(), settings_.end(), pSettings );

	if (it != settings_.end())
		settings_.erase( it );
}

BW_END_NAMESPACE

// terrain_graphics_options.cpp
