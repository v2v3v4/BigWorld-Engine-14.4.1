#ifndef VERTEX_DECLARATION_HPP
#define VERTEX_DECLARATION_HPP

#include "moo_dx.hpp"
#include "com_object_wrap.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/concurrency.hpp"
#include "vertex_format.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

typedef ComObjectWrap< DX::VertexDeclaration > D3DVertexDeclarationPtr;

/**
 * This class handles the vertex declarations used by moo. Vertex declarations 
 * encapsulate the way VertexFormats are sent to the graphics device. 
 * Vertex formats are stored on disk (in res\\shaders\\formats ), a subset of which
 * can be supported by this class as VertexDeclarations, depending on the
 * graphics target device.
 *
 * For more information please see the vertex formats section of the 
 * client programming guide.
 */
class VertexDeclaration
{
public:
	DX::VertexDeclaration* declaration() const { return pDecl_.pComObject(); }

	const BW::string& name() const { return name_; }
	const VertexFormat& format() const {return format_; }

	static VertexDeclaration* combine( VertexDeclaration* orig, VertexDeclaration* extra );
	static VertexDeclaration* get( const BW::StringRef & declName );
	static void fini();

	static bool isTypeSupportedOnDevice( 
		const VertexElement::StorageType::Value & storageType );
	static bool isFormatSupportedOnDevice( const VertexFormat & format );
	static const char * getTargetDevice();
	static void setTargetDevice( const char * target );

private:
	D3DVertexDeclarationPtr	pDecl_;
	VertexDeclaration( const VertexDeclaration& );
	VertexDeclaration& operator=( const VertexDeclaration& );
	VertexDeclaration( const BW::string& name );
	~VertexDeclaration();

	bool	load( DataSectionPtr pSection, const BW::StringRef & declName );
	bool	merge( VertexDeclaration* orig, VertexDeclaration* extra );
	bool    createDeclaration();

	BW::string name_;
	VertexFormat format_;

	static SimpleMutex	declarationsLock_;
	static char VertexDeclaration::s_targetDevice[16];

};

} // namespace Moo

BW_END_NAMESPACE

#endif // VERTEX_DECLARATION_HPP
