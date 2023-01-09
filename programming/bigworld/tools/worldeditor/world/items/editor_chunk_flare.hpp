#ifndef EDITOR_CHUNK_FLARE_HPP
#define EDITOR_CHUNK_FLARE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_flare.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a ChunkFlare
 */
class EditorChunkFlare : public EditorChunkSubstance<ChunkFlare>
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkFlare )
public:
	EditorChunkFlare();
	~EditorChunkFlare();

	bool load( DataSectionPtr pSection, Chunk* pChunk, BW::string* errorString = NULL );

	virtual bool edSave( DataSectionPtr pSection );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual bool edEdit( class GeneralEditor & editor );

	virtual void tick( float /*dTime*/ );

	virtual void draw( Moo::DrawContext& drawContext );

	Moo::Colour colour() const;
	void colour( const Moo::Colour & c );

	BW::string flareResGet() const				{ return flareRes_; }
	void flareResSet( const BW::string & res )	{ flareRes_ = res; }

	float getMaxDistance() const;
	bool setMaxDistance( const float& maxDistance );

	float getArea() const;
	bool setArea( const float& area );

	float getFadeSpeed() const;
	bool setFadeSpeed( const float& fadeSpeed );

	virtual void syncInit();

	virtual bool edIsSnappable() { return false; }

	virtual BW::string edAssetName() const { return "Flare"; }

private:
	EditorChunkFlare( const EditorChunkFlare& );
	EditorChunkFlare& operator=( const EditorChunkFlare& );

	virtual const char * sectName() const	{ return "flare"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::flaresVisible(); }
	virtual const char * drawFlag() const
		{ return "render/scenery/drawChunkFlares"; }
	virtual ModelPtr reprModel() const;

	ModelPtr flareModel_;		// Large proxy
	ModelPtr flareModelSmall_;	//Small proxy

	BW::string		flareRes_;

	Matrix			transform_;

	// Pointer to whichever of the big small icons umbra should use. (also current, for switching)
	ModelPtr	currentModel_;
};


typedef SmartPointer<EditorChunkFlare> EditorChunkFlarePtr;


BW_END_NAMESPACE

#endif // EDITOR_CHUNK_FLARE_HPP
