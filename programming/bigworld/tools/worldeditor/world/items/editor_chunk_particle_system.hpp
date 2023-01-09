#ifndef EDITOR_CHUNK_PARTICLE_SYSTEM_HPP
#define EDITOR_CHUNK_PARTICLE_SYSTEM_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "particle/chunk_particles.hpp"

#include "editor_chunk_substance.hpp"

#include "cstdmf/bw_string.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a ChunkParticleSystem
 */
class EditorChunkParticleSystem : public EditorChunkSubstance<ChunkParticles>
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorParticleSystem )

	static BW::map<BW::string, BW::set<EditorChunkParticleSystem*> > editorChunkParticleSystem_;
	static void add( EditorChunkParticleSystem*, const BW::string& resourceName );
	static void remove( EditorChunkParticleSystem* );
	using ChunkParticles::load;

public:
	static void reload( const BW::string& filename );

	EditorChunkParticleSystem();
	~EditorChunkParticleSystem();

	bool load( DataSectionPtr pSection, Chunk* chunk, BW::string* errorString = NULL );

	virtual void tick( float dTime );

	virtual bool edShouldDraw();
	virtual void draw( Moo::DrawContext& drawContext );

	virtual bool edSave( DataSectionPtr pSection );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual void edWorldBounds( BoundingBox & bbRet );

	virtual void syncInit() { needsSyncInit_ = true; }

	virtual bool edIsSnappable() { return false; }

	virtual bool edEdit( class GeneralEditor & editor );

	virtual BW::string edAssetName() const { return BWResource::getFilename( resourceName_ ).to_string(); }
	virtual BW::string edFilePath() const { return resourceName_; }

	virtual BW::vector<BW::string> edCommand( const BW::string& path ) const;
	virtual bool edExecuteCommand( const BW::string& path, BW::vector<BW::string>::size_type index );

	virtual void drawBoundingBoxes( const BoundingBox &bb, const BoundingBox &vbb, const Matrix &spaceTrans ) const;

	virtual bool addYBounds( BoundingBox& bb ) const;

private:
	ModelPtr largeProxy();

	EditorChunkParticleSystem( const EditorChunkParticleSystem& );
	EditorChunkParticleSystem& operator=( const EditorChunkParticleSystem& );

	virtual const char * sectName() const	{ return "particles"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::particlesVisible(); }
	virtual const char * drawFlag() const
		{ return "render/scenery/particle"; }
	virtual ModelPtr reprModel() const;

	ModelPtr psModel_;			//Large proxy
	ModelPtr psModelSmall_;		//Small proxy
	ModelPtr psBadModel_;
	ModelPtr overrideModel_;
	BW::string overrideModelName_;
	BW::vector<Matrix>	helperModelHardPointTransforms_;

	BW::string		resourceName_;
	DataSectionPtr  pOriginalSect_;

	mutable ModelPtr currentModel_;
	mutable bool needsSyncInit_;
};


typedef SmartPointer<EditorChunkParticleSystem> EditorChunkParticleSystemPtr;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_PARTICLE_SYSTEM_HPP
