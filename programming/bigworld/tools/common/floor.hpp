#ifndef _FLOOR_H_
#define _FLOOR_H_

#include "moo/forward_declarations.hpp"
#include "moo/moo_math.hpp"
#include "moo/custom_mesh.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/complex_effect_material.hpp"
/*#include "moo/render_context.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/effect_material.hpp"
#include "moo/texture_manager.hpp"*/

/**
 * Floor class.
 *
 * Used by the editors, this class provides the following options:
 *
 * Opacity
 * Floor Texture
 * Texture Tiling options
 */

BW_BEGIN_NAMESPACE

class Floor
{
public:
	explicit Floor( const BW::string& textureName = "" );
	~Floor();

    void			render( Moo::ERenderingPassType renderingPassType = Moo::RENDERING_PASS_COLOR );

    void			setTextureName( const BW::string &textureFileName );
    BW::string&	getTextureName( void );

    void			visible( bool state )	{ visible_ = state; }
    bool			visible() const			{ return visible_; }

    void			location( const Vector3 & l );
    const Vector3&	location() const;

private:
    void drawSquare( Moo::VertexXYZNUVC& v, float x, float z, float step, float scale );

	//private methods
    void	updateMaterial( void );
    void	updateMesh( void );
    void	cleanupMaterial( void );

    //serializable options
    BW::string		textureName_;
    bool			visible_;
    Matrix			transform_;

    //transient variables
    CustomMesh< Moo::VertexXYZNUVC >	mesh_;
	Moo::VertexDeclaration*				pMeshDclr_;
    bool								meshDirty_;

    Moo::ComplexEffectMaterialPtr	material_;
    bool					materialDirty_;
};

BW_END_NAMESPACE
#endif
