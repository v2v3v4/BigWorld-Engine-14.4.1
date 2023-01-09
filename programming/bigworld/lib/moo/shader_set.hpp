#ifndef SHADER_SET_HPP
#define SHADER_SET_HPP

#include "resmgr/datasection.hpp"
#include "cstdmf/smartpointer.hpp"
#include "device_callback.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer< class ShaderSet > ShaderSetPtr;

/**
 * This class contains a set of shaders based on different light combinations,
 * and the same base shader.
 */
class ShaderSet : public Moo::DeviceCallback, public ReferenceCount
{
public:
	typedef uint32 ShaderID;
	typedef IDirect3DVertexShader9* ShaderHandle;

	typedef BW::map< ShaderID, ShaderHandle > ShaderMap;

	ShaderSet( DataSectionPtr shaderSetSection, const BW::string & vertexFormat, 
		const BW::string & shaderType );
	~ShaderSet();

	static ShaderID		shaderID( char nDirectionalLights, char nPointLights, char nSpotLights );

	ShaderHandle		shader( char nDirectionalLights, char nPointLights, char nSpotLights, bool hardwareVP = false );
	ShaderHandle		shader( ShaderID shaderID, bool hardwareVP = false );

	void				deleteUnmanagedObjects( );

	bool				isSameSet( const BW::string & vertexFormat, const BW::string & shaderType )
		{ return vertexFormat == vertexFormat_ && shaderType == shaderType_; }

	void				preloadAll();

private:

	ShaderMap		shaders_;
	ShaderMap		hwShaders_;

	DataSectionPtr	shaderSetSection_;

	BW::string		vertexFormat_;
	BW::string		shaderType_;

	bool			preloading_;

	ShaderSet(const ShaderSet&);
	ShaderSet& operator=(const ShaderSet&);
};

#ifdef CODE_INLINE
#include "shader_set.ipp"
#endif

BW_END_NAMESPACE

#endif
