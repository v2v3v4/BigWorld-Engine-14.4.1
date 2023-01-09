#pragma once

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_deferred_decal.hpp"
#include "editor_chunk_substance.hpp"

BW_BEGIN_NAMESPACE

//-- Editor representation of StaticChunkDecal.
//--------------------------------------------------------------------------------------------------
class EditorStaticChunkDecal : public EditorChunkSubstance<StaticChunkDecal>
{
    DECLARE_EDITOR_CHUNK_ITEM(EditorStaticChunkDecal)

public:
    EditorStaticChunkDecal();
    virtual ~EditorStaticChunkDecal();

    bool                    load(DataSectionPtr iData);
    bool                    load(DataSectionPtr iData, Chunk* pChunk);
    bool                    reload();

//  virtual bool            needsUpdate( uint32 frameTimestamp ) const;
    virtual void            updateAnimations( /*uint32 frameTimestamp*/ );
    virtual void            draw( Moo::DrawContext& drawContext );
    virtual bool            edEdit(class GeneralEditor& editor);
    virtual bool            edSave(DataSectionPtr iData);

    virtual bool            edTransform(const Matrix& m, bool transient);
    virtual const Matrix&   edTransform();
    virtual void            edGetTexturesAndVertices( BW::map<void*, int>& texturesInfo, BW::map<void*, int>& verticesInfo );
    virtual int             edTextureMemory() const;
    virtual int             edVertexBufferMemory() const;
    virtual BW::string      edFilePath() const;
    virtual bool edIsToBeCounted() { return false; }

    //-- EditorChunkSubstance interface.
    virtual const char*     sectName() const;
    virtual bool isDrawFlagVisible() const;
    virtual const char*     drawFlag() const;
    virtual ModelPtr        reprModel() const;
    virtual void            addAsObstacle();

    //-- properties access.
    bool                    setGenericApplyTo(uint type, bool enable);
    bool                    getGenericApplyTo(uint type) const;

    //-- 
    template <DecalsManager::Decal::EInfluenceType Type>
    bool                    setApplyTo(const bool& flag) { return setGenericApplyTo(static_cast<uint>(Type), flag); }

    template <DecalsManager::Decal::EInfluenceType Type>
    bool                    getApplyTo() const           { return getGenericApplyTo(static_cast<uint>(Type)); }

    bool                    setMaterialType(const uint& material);
    uint                    getMaterialType() const { return m_decalDesc.m_materialType; }

    bool                    setPriority(const uint& priority);
    uint                    getPriority() const { return m_decalDesc.m_priority; }

    void                    setMap1(const BW::string& texture);
    BW::string              getMap1() const { return m_decalDesc.m_map1; } 

    void                    setMap2(const BW::string& texture);
    BW::string              getMap2() const { return m_decalDesc.m_map2; }

    void                    minimizeBoxSize();

private:
    void updateDecalProps();

private:
    bool        map1Changed;
    ModelPtr    m_model;
    ModelPtr    m_missingDecalModel;
};

BW_END_NAMESPACE
