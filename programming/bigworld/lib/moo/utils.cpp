#include "pch.hpp"


#include "utils.hpp"
#include "moo\effect_visual_context.hpp"

BW_BEGIN_NAMESPACE

float savedNearPlane_,savedFarPlane_;

void beginZBIASDraw(float bias)
{
#ifndef EDITOR_ENABLED
    Moo::RenderContext& rc = Moo::rc();
    savedNearPlane_ = rc.camera().nearPlane();
    savedFarPlane_  = rc.camera().farPlane();
    rc.camera().nearPlane( savedNearPlane_ * bias );
    rc.camera().farPlane(  savedFarPlane_ * bias );
    rc.updateProjectionMatrix();
    rc.updateViewTransforms();

    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_VIEW);
#endif
}

void endZBIASDraw()
{
#ifndef EDITOR_ENABLED
    Moo::RenderContext& rc = Moo::rc();
    rc.camera().nearPlane( savedNearPlane_ );
    rc.camera().farPlane( savedFarPlane_ );
    rc.updateProjectionMatrix();
    rc.updateViewTransforms();

    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_VIEW);
#endif
}

void calculateNormals(BW::vector< CPUSpotVertex > &vertices)
{
    // calculate normals for polygons
    for(size_t i = 0; i < vertices.size()/3; ++i)
    {
        const size_t v = i*3;
        Vector3 normal = (vertices[v+1].pos_-vertices[v].pos_).crossProduct(vertices[v+2].pos_-vertices[v+1].pos_);
        normal.normalise();
        vertices[v].normal_ = normal;
        vertices[v+1].normal_ = normal;
        vertices[v+2].normal_ = normal;
    }
    // normals averaging - to get normals for vertices
    BW::vector<bool> isAveraged;
    isAveraged.resize(vertices.size(),false);
    if(vertices.empty()) return;
    for(size_t i = 0; i < vertices.size()-1; ++i)
    {
        if(isAveraged[i]) continue;
        isAveraged[i] = true;
        Vector3 normal = vertices[i].normal_;
        BW::vector<size_t> copies;
        copies.reserve(20);
        copies.push_back(i);
        for(size_t j = i+1; j < vertices.size(); ++j)
        {
            if(vertices[i].pos_ == vertices[j].pos_)//isEqual(vertices[i].pos_,vertices[j].pos_))
            {
                isAveraged[j] = true;
                normal = normal+vertices[j].normal_;
                copies.push_back(j);
            }
        }
        normal = normal/static_cast<float>(copies.size());
        normal.normalise();
        for(size_t k = 0; k < copies.size(); ++k)
        {
            vertices[copies[k]].normal_ = normal;
        }
    }
}

BW_END_NAMESPACE
