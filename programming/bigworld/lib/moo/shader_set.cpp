#include "pch.hpp"

#include <algorithm>

#include "shader_set.hpp"
#include "render_context.hpp"
#include "cstdmf/debug.hpp"

#ifndef CODE_INLINE
#include "shader_set.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

/**
 * Construct shader set.
 */
ShaderSet::ShaderSet( DataSectionPtr shaderSetSection, const BW::string & vertexFormat, 
					 const BW::string & shaderType )
: shaderSetSection_( shaderSetSection ),
  vertexFormat_( vertexFormat ),
  shaderType_( shaderType ),
  preloading_( false )
{
	BW_GUARD;
}

/**
 * Destructor.
 */
ShaderSet::~ShaderSet()
{
	BW_GUARD;
	deleteUnmanagedObjects();
}

/**
 * Generate a shader id based on number of lights.
 */
ShaderSet::ShaderID ShaderSet::shaderID( char nDirectionalLights, char nPointLights, char nSpotLights )
{
	ShaderID sid = 0;

	sid = nSpotLights + '0';
	sid <<= 8;
	sid |= nPointLights + '0';
	sid <<= 8;
	sid |= nDirectionalLights + '0';

	return sid;
}

/**
 * Return a handle to a shader created from a combination of lights.
 */
ShaderSet::ShaderHandle ShaderSet::shader( char nDirectionalLights, char nPointLights, char nSpotLights, bool hardwareVP )
{
	ShaderID sid = shaderID( nDirectionalLights, nPointLights, nSpotLights );

	return shader( sid, hardwareVP );
}

/**
* Return a handle to a shader created a shader ID.
*/
ShaderSet::ShaderHandle ShaderSet::shader( ShaderSet::ShaderID shaderID, bool hardwareVP )
{
	BW_GUARD;
	if( Moo::rc().device() == (void*)NULL )
	{
		CRITICAL_MSG( "Trying to create a shader before the device has been created!" );
	}

	ShaderHandle sh = 0;
	if (!hardwareVP)
	{
		// Try to find the shader.
		ShaderMap::iterator it = shaders_.find( shaderID );

		// Do we need to create the shader?
		if( it == shaders_.end() )
		{
			// Create the shader name.
			BW::string s( "0d0o0s.vso" );
			s[0] = char( shaderID );
			s[2] = char( shaderID >> 8 );
			s[4] = char( shaderID >> 16 );

			// Try to load the shader.
			BinaryPtr shaderBin = shaderSetSection_->readBinary( s );
			if( shaderBin )
			{
				// Try to create the shader.
				HRESULT res = Moo::rc().device()->CreateVertexShader(
					(DWORD*)shaderBin->data(), &sh );

				//D3DUSAGE_SOFTWAREPROCESSING );

				if( SUCCEEDED( res ) )
				{
					// Shader creation was a success add it to the shadermap.
					shaders_[ shaderID ] = sh;
				}
				else
					sh = 0;
			}
			else if (!preloading_)
			{
				CRITICAL_MSG( "Couldn't find shader: %s/%s/%s\n",
					vertexFormat_.c_str(),
					shaderType_.c_str(),
					s.c_str() );
			}
		}
		else
		{
			sh = it->second;
		}
	}
	else
	{
		// Try to find the shader.
		ShaderMap::iterator it = hwShaders_.find( shaderID );

		// Do we need to create the shader?
		if( it == hwShaders_.end() )
		{
			// Create the shader name.
			BW::string s( "0d0o0s.vso" );
			s[0] = char( shaderID );
			s[2] = char( shaderID >> 8 );
			s[4] = char( shaderID >> 16 );

			// Try to load the shader.
			BinaryPtr shaderBin = shaderSetSection_->readBinary( s );
			if( shaderBin )
			{
				// Try to create the shader.
				HRESULT res = Moo::rc().device()->CreateVertexShader( (DWORD*)shaderBin->data(), &sh );
				if( SUCCEEDED( res ) )
				{
					// Shader creation was a success add it to the shadermap.
					hwShaders_[ shaderID ] = sh;
				}
				else
					sh = 0;
			}
			else if (!preloading_)
			{
				CRITICAL_MSG( "Couldn't find shader: %s/%s/%s\n",
					vertexFormat_.c_str(),
					shaderType_.c_str(),
					s.c_str() );
			}
		}
		else
		{
			sh = it->second;
		}

	}

	return sh;
}

/**
 * Destroy low-level resources.
 */
void ShaderSet::deleteUnmanagedObjects( )
{
	BW_GUARD;

	ShaderMap::iterator it = shaders_.begin();
	ShaderMap::iterator end = shaders_.end();

	// iterate through the shadermap and delete the shaders.
	while( it != end )
	{
		ShaderMap::iterator nit = it++;
		if( Moo::rc().device() != (void*)NULL )
		{
			nit->second->Release();
			shaders_.erase( nit );
		}
	}

	it = hwShaders_.begin();
	end = hwShaders_.end();

	// iterate through the shadermap and delete the shaders.
	while( it != end )
	{
		ShaderMap::iterator nit = it++;
		if( Moo::rc().device() != (void*)NULL )
		{
			nit->second->Release();
			hwShaders_.erase( nit );
		}
	}
}


/**
 *	Preload all light permutations of this shader set
 */
void ShaderSet::preloadAll()
{
	BW_GUARD;
	preloading_ = true;
//	for (uint i = 0; i < 2 * 5 * 4; i++)
	for (uint d = 0; d < 2; d++)
	{
		for (uint o = 0; o < 5; o++)
		{
			for (uint s = 0; s < 3; s++)
			{
				// xbox version only uses hardware shaders
				this->shader( d, o, s, true );
			}
		}
	}
	preloading_ = false;
}

BW_END_NAMESPACE

// shader_set.cpp
