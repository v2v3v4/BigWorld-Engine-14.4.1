#pragma once

#include "chunk\chunk.hpp"
#include "chunk\chunk_item.hpp"
#include "moo\deferred_decals_manager.hpp"

BW_BEGIN_NAMESPACE

//-- Represents static decal which never changes its position and any properties.
//--------------------------------------------------------------------------------------------------
class StaticChunkDecal : public ChunkItem
{
    DECLARE_CHUNK_ITEM(StaticChunkDecal)

public:
    StaticChunkDecal();
    virtual ~StaticChunkDecal();
    
    bool            load(DataSectionPtr iData);
    bool            load(DataSectionPtr iData, Chunk* pChunk);
    virtual void    toss(Chunk* pChunk);
    virtual void    draw( Moo::DrawContext& drawContext );
    virtual void    syncInit();

    void            update(const Matrix& mat);
    bool            isLoaded() const { return m_decal != -1; }

protected:
    DecalsManager::Decal::Desc  m_decalDesc;
    int32                       m_decal;
    BoundingBox                 m_worldBounds;
};

extern int ChunkDeferredDecal_token;

BW_END_NAMESPACE
