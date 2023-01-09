#include "pch.hpp"
#include "material_preview.hpp"

#include "moo/complex_effect_material.hpp"
#include "moo/render_target.hpp"
#include "moo/texture_manager.hpp"
#include "page_materials.hpp"
#include "moo/geometrics.hpp"

#include "resmgr/bwresource.hpp"

#include "me_app.hpp"

#include "modeleditor_core/Models/mutant.hpp"

DECLARE_DEBUG_COMPONENT2( "MaterialPreview", 0 )

BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( MaterialPreview )


/**
 *	Constructor
 */
MaterialPreview::MaterialPreview()
{
}

		
//==============================================================================
BinaryPtr MaterialPreview::generatePreview(
	const BW::string & materialName,
	const BW::string & matterName,
	const BW::string & tintName )
{
	ComObjectWrap< ID3DXBuffer > buffer;

	//Grab diffuse map file location as string
	BW::string textureString = MeApp::instance().mutant()->materialPropertyVal(
		materialName,
		matterName,
		tintName,
		"diffuseMap",
		"Texture" );

	bool conversionSuccess = false;

	// If there is a diffuse map convert it to .bmp
	if (!textureString.empty())
	{
		// Get a lockable version of the texture or D3DXSaveTextureToFile will
		// fail
		Moo::BaseTexturePtr pLockableTexture =
			Moo::TextureManager::instance()->getSystemMemoryTexture(
			textureString );

		// Only generate material preview if the texture loaded successfully
		if (pLockableTexture.exists())
		{
			// Save
			HRESULT hr = D3DXSaveTextureToFileInMemory( &buffer,
				D3DXIFF_BMP, pLockableTexture->pTexture(), NULL );

			// Save failed
			if (FAILED( hr ))
			{
				WARNING_MSG( "Could not create material preview file "
					"(D3D error = %s).\n",
					DX::errorAsString( hr ).c_str() );
			}
			else
			{
				conversionSuccess = true;
			}
		}
	}
	// Else try to grab its colour and render it to the .bmp,
	// if that fails render white.
	if (!conversionSuccess)
	{
		BW::string colourString = MeApp::instance().mutant()->materialPropertyVal(
			materialName,
			matterName,
			tintName,
			"Color",
			"Vector4" );

		if (colourString == "")
		{
			colourString = MeApp::instance().mutant()->materialPropertyVal(
				materialName,
				matterName,
				tintName,
				"Colour",
				"Vector4" );
		}

		if (colourString == "")
		{
			colourString = "1 1 1 0";
		}

		Moo::Colour col;
		sscanf( colourString.c_str(), "%f %f %f %f", &col.r, &col.g, &col.b, &col.a );
	
		if (!rt_)
		{
			rt_ = new Moo::RenderTarget( "Material Preview" );
			if (!rt_ || !rt_->create( 128,128 ))
			{
				WARNING_MSG( "Could not create render target for material preview.\n" );
				rt_ = NULL;
				return NULL;
			}
		}

		if (rt_->push())
		{
			Moo::rc().beginScene();
		
			Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
				RGB( 255, 255, 255), 1, 0 );
			
			Geometrics::drawRect(
				Vector2( 0, 0 ),
				Vector2( 128, 128 ),
				col );

			Moo::rc().endScene();
			
			HRESULT hr = D3DXSaveTextureToFileInMemory( &buffer,
				D3DXIFF_BMP, rt_->pTexture(), NULL );
					
				if (FAILED( hr ))
				{
					WARNING_MSG( "Could not create material preview file (D3D error = 0x%x).\n", hr);
				rt_->pop();
				return NULL;
			}

			rt_->pop();

		}
	}

	return new BinaryBlock( buffer->GetBufferPointer(),
		buffer->GetBufferSize(), "BinaryBlock/BaseTextureSave" );
}
BW_END_NAMESPACE

