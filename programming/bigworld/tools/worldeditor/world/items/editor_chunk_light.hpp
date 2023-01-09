#ifndef EDITOR_CHUNK_LIGHT_HPP
#define EDITOR_CHUNK_LIGHT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "common/lighting_influence.hpp"
#include "chunk/chunk_light.hpp"

BW_BEGIN_NAMESPACE

/**
 * We extend Aligned here, as all the editor chunk lights have Matrix members
 */
template<class BaseLight>
class EditorChunkLight : public EditorChunkSubstance<BaseLight>
{
public:
	EditorChunkLight();

	virtual void edPreDelete()
	{
		BW_GUARD;

		markInfluencedChunks();
		EditorChunkSubstance<BaseLight>::edPreDelete();
	}

	virtual void edPostCreate()
	{
		BW_GUARD;

		markInfluencedChunks();
		EditorChunkSubstance<BaseLight>::edPostCreate();
	}

	virtual bool edShouldDraw()
	{
		BW_GUARD;

		// Override EditorChunkSubstance's method since it's tied to 'Scenery'.
		return BaseLight::edShouldDraw();
	}

	virtual void markInfluencedChunks()
	{
		BW_GUARD;
	}

	virtual void markDynamicInfluencedChunks()
	{
		BW_GUARD;
	}

	virtual void markStaticInfluencedChunks()
	{
		BW_GUARD;
	}
	

	virtual bool load( DataSectionPtr pSection )
	{
		BW_GUARD;

		if (EditorChunkSubstance<BaseLight>::load( pSection ))
		{
			this->loadModel();
			this->updateReprModel();
			return true;
		}
		else
		{
			return false;
		}
	}
	
	virtual void draw( Moo::DrawContext& drawContext )
	{
		BW_GUARD;

		if (!edIsTooDistant() || WorldManager::instance().drawSelection())
		{
			// Draw the light proxy if it's not too far away from teh camera.
			EditorChunkSubstance<BaseLight>::draw( drawContext );
		}
	}

	virtual bool usesLargeProxy() const = 0;

	ModelPtr reprModel() const 
	{ 
		BW_GUARD;

		if (usesLargeProxy())
		{
			return model_; 
		}
		else
		{
			return modelSmall_; 
		}

		return NULL;
	}
	
	virtual void syncInit();

	virtual bool edIsSnappable() { return false; }

	virtual BW::string edAssetName() const { return sectName(); }

protected:
	ModelPtr model_;
	ModelPtr modelSmall_;
	virtual void loadModel() = 0;
	Matrix	transform_;
};


/**
 * A ChunkLight containing a light that moo knows about, ie everything but
 * ambient
 */
template<class BaseLight>
class EditorChunkMooLight : public EditorChunkLight<BaseLight>
{
public:
	EditorChunkMooLight()  {}

	virtual void toss( Chunk * pChunk )
	{
		BW_GUARD;

		this->EditorChunkSubstance<BaseLight>::toss( pChunk );
	}

	virtual bool load( DataSectionPtr pSection )
	{
		BW_GUARD;

		return EditorChunkLight<BaseLight>::load( pSection );
	}

	virtual bool edShouldDraw()
	{
		BW_GUARD;

		if( !EditorChunkLight<BaseLight>::edShouldDraw() )
			return false;
		if (!OptionsLightProxies::visible())
			return false;

		return OptionsLightProxies::dynamicVisible();
	}
};


/**
 * A ChunkLight containing a light that moo knows about, and exists somewhere
 * in the world, ie, neither ambient nor directional
 */
template<class BaseLight>
class EditorChunkPhysicalMooLight : public EditorChunkMooLight<BaseLight>
{
public:

	virtual void markInfluencedChunks()
	{
		BW_GUARD;
		if (pChunk_)
		{
			LightingInfluence::DynamicLightMarkChunksVisitor<BaseLight*> visitor(
				this );
			LightingInfluence::visitChunks( pChunk_, &visitor);
		}
	}
};


/**
 *	This class is the editor version of a chunk directional light
 */
class EditorChunkDirectionalLight :
	public EditorChunkMooLight<ChunkDirectionalLight>
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkDirectionalLight )
public:

	virtual bool load( DataSectionPtr pSection );
	virtual bool edSave( DataSectionPtr pSection );

	virtual bool edEdit( class GeneralEditor & editor );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual const char * sectName() const { return "directionalLight"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::lightsVisible(); }
	virtual const char * drawFlag() const
		{ return "render/scenery/drawChunkLights"; }

	virtual bool usesLargeProxy() const { return false; }
	
	Moo::DirectionalLightPtr	pLight()	{ return pLight_; }

	float getMultiplier() const { return pLight_->multiplier(); }
	bool setMultiplier( const float& m ) { pLight_->multiplier(m); markInfluencedChunks(); return true; }

protected:
	virtual void loadModel();

};


/**
 *	This class is the editor version of a chunk omni light
 */
class EditorChunkOmniLight : public EditorChunkPhysicalMooLight<ChunkOmniLight>
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkOmniLight )
public:

	virtual bool load( DataSectionPtr pSection );
	virtual bool edSave( DataSectionPtr pSection );

	virtual bool edEdit( class GeneralEditor & editor );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual const char * sectName() const { return "omniLight"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::lightsVisible(); }
	virtual const char * drawFlag() const
		{ return "render/scenery/drawChunkLights"; }

	virtual bool usesLargeProxy() const;
	
	Moo::OmniLightPtr	pLight()	{ return pLight_; }

	float getMultiplier() const { return pLight_->multiplier(); }
	bool setMultiplier( const float& m ) { pLight_->multiplier(m); markInfluencedChunks(); return true; }

	bool setPriority( const int& priority ) { pLight_->priority(priority); markInfluencedChunks(); return true; }
	int getPriority() const { return pLight_->priority(); }

protected:
	virtual void loadModel();

};


/**
 *	This class is the editor version of a chunk spot light
 */
class EditorChunkSpotLight : public EditorChunkPhysicalMooLight<ChunkSpotLight>
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkSpotLight )
public:

	virtual bool load( DataSectionPtr pSection );
	virtual bool edSave( DataSectionPtr pSection );

	virtual bool edEdit( class GeneralEditor & editor );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual const char * sectName() const { return "spotLight"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::lightsVisible(); }
	virtual const char * drawFlag() const
		{ return "render/scenery/drawChunkLights"; }

	virtual bool usesLargeProxy() const;
	
	Moo::SpotLightPtr	pLight()	{ return pLight_; }

	float getMultiplier() const { return pLight_->multiplier(); }
	bool setMultiplier( const float& m ) { pLight_->multiplier(m); markInfluencedChunks(); return true; }

	bool setPriority( const int& priority ) { pLight_->priority(priority); markInfluencedChunks(); return true; }
	int getPriority() const { return pLight_->priority(); }

	virtual bool edShouldDraw();

protected:
	virtual void loadModel();

};


/**
 *	This class is the editor version of a chunk omni light
 */
class EditorChunkPulseLight : public EditorChunkPhysicalMooLight<ChunkPulseLight>
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkPulseLight )
public:
	typedef BW::vector<Vector2> Frames;

	virtual bool load( DataSectionPtr pSection );
	virtual bool edSave( DataSectionPtr pSection );

	virtual bool edEdit( class GeneralEditor & editor );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual const char * sectName() const { return "pulseLight"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::lightsVisible(); }
	virtual const char * drawFlag() const
		{ return "render/scenery/drawChunkLights"; }

	virtual bool usesLargeProxy() const;
	
	Moo::OmniLightPtr	pLight()	{ return pLight_; }

	float getMultiplier() const { return pLight_->multiplier(); }
	bool setMultiplier( const float& m ) { pLight_->multiplier(m); markInfluencedChunks(); return true; }

	bool setPriority( const int& priority ) { pLight_->priority(priority); markInfluencedChunks(); return true; }
	int getPriority() const { return pLight_->priority(); }

	float getTimeScale() const	{ return timeScale_; }
	bool setTimeScale( const float& timeScale );

	float getDuration() const	{ return duration_; }
	bool setDuration( const float& duration );

	BW::string getAnimation() const	{ return animation_; }
	bool setAnimation( const BW::string& animation );

	Frames& getFrames()	{ return frames_;	}
	bool setFrames( const Frames& frames );

	void onFrameChanged();

	virtual bool edShouldDraw();

protected:
	virtual void loadModel();

	float timeScale_;
	float duration_;
	BW::string animation_;
	Frames frames_;
};


/**
 *	This class is the editor version of a chunk ambient light
 */

class EditorChunkAmbientLight :
	public EditorChunkLight<ChunkAmbientLight>
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkAmbientLight )
public:

	bool load( DataSectionPtr pSection );
	virtual bool edSave( DataSectionPtr pSection );

	virtual bool edEdit( class GeneralEditor & editor );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual const char * sectName() const { return "ambientLight"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::lightsVisible(); }
	virtual const char * drawFlag() const
		{ return "render/scenery/drawChunkLights"; }

	virtual bool usesLargeProxy() const;
	
	EditorChunkAmbientLight *	pLight()	{ return this; }

	const Moo::Colour & colour() const			{ return colour_; }
	void colour( const Moo::Colour & c );

	virtual void toss( Chunk * pChunk );

	float getMultiplier() const { return multiplier(); }
	bool setMultiplier( const float& m );

	virtual bool edShouldDraw();

	virtual void markInfluencedChunks();


protected:
	virtual void loadModel();

};


BW_END_NAMESPACE

#endif // EDITOR_CHUNK_LIGHT_HPP
