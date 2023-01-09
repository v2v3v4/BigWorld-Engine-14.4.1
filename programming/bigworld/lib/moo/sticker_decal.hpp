#pragma once

#include "cstdmf\smartpointer.hpp"
#include "terrain\terrain2\terrain_height_map2.hpp"
#include "duplo\py_attachment.hpp"
#include "deferred_decals_manager.hpp"

BW_BEGIN_NAMESPACE

class ForwardStickerModel;
class PyModel;
typedef ScriptObjectPtr<PyModel> PyModelPtr;
#ifndef PyModel_CONVERTERS
PY_SCRIPT_CONVERTERS_DECLARE( PyModel )
#define PyModel_CONVERTERS
#endif

namespace Moo
{
    class SysMemTexture;
}

class StickerDecal
{
    DecalsManager::Decal::Desc desc_;
    uint32 idx_;
public:
    StickerDecal(DecalsManager::Decal::Desc desc, uint32 idx): idx_(idx), desc_(desc) {}
    uint32 Index() { return idx_; }
    inline const Matrix& transform() const { return desc_.m_transform; }
};

class PyStickerModel : public PyAttachment
{
private:

    ForwardStickerModel* pForwardModel;

    BW::map<uint32, StickerDecal> decals_;

    float lodDistances[2];
    float alphas[2];
    const PyModel* owner;
    SmartPointer<Moo::SysMemTexture> pSrcTex_;
    uint32 updateTimestamp_;

    Py_Header( PyStickerModel, PyAttachment );
    static bool s_bFirstInit;
    static float s_criticalAngle;
public:
    static bool s_enabledDrawing;
    static void criticalAngle(float angle) { s_criticalAngle = angle; };
    static float criticalAngle() { return s_criticalAngle; };

    PyStickerModel( PyTypePlus * pType = &s_type_ );
    virtual ~PyStickerModel();
    virtual bool isLodVisible() const { return true; }
    virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const 
    { return false; }
    virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
    { return false; }
    virtual void tick( float dTime );
    virtual bool needsUpdate() const;
    virtual void updateAnimations( const Matrix & worldTransform, float lod );
    virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );
    virtual void tossed( bool outside );
    static PyStickerModel * New();
    PY_AUTO_FACTORY_DECLARE(PyStickerModel, END )
        int addSticker( BW::string texture, BW::string bumpTexture, const PyModelPtr& model_, 
                        Vector3 segmentStart, Vector3 segmentEnd, Vector2 size_, Vector3 up_, bool isInscription);
    PY_AUTO_METHOD_DECLARE( RETDATA, addSticker, 
        OPTARG( BW::string, "", 
        OPTARG( BW::string, "", 
        ARG( PyModelPtr,  
        OPTARG( Vector3, Vector3(0,0,0), 
        OPTARG( Vector3, Vector3(0,0,0), 
        OPTARG( Vector2, Vector2(0,0), 
        OPTARG( Vector3, Vector3(0,1,0), 
        OPTARG( bool, false, 
        END ) ) ) ) ) ) ) ) )

        void delSticker(int stickerID);
    PY_AUTO_METHOD_DECLARE( RETVOID, delSticker, ARG( int, END) )
        void clear();
    PY_AUTO_METHOD_DECLARE( RETVOID, clear, END )
        void setLODDistances(float texture0distance,float texture1distance);
    PY_AUTO_METHOD_DECLARE( RETVOID, setLODDistances, ARG( float, ARG(float, END) ) );
        void setAlphas(float texture0alpha,float texture1alpha);
    PY_AUTO_METHOD_DECLARE( RETVOID, setAlphas, ARG( float, ARG(float, END) ) );
        void setTextureData(PyObjectPtr data);
    PY_AUTO_METHOD_DECLARE(RETVOID, setTextureData, ARG(PyObjectPtr, END));

};

BW_END_NAMESPACE
