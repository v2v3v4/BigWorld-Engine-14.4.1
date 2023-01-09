#ifndef ULTILITIES_HPP
#define ULTILITIES_HPP

#include "cstdmf/bw_string.hpp"

#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "math/matrix.hpp"

class CWnd;
class CEdit;

BW_BEGIN_NAMESPACE

class Chunk;
class GeometryMapping;

class Utilities
{
public:
	static POINT move( CWnd& widget, int x, int y );
	static void moveAndSize( CWnd& widget, int x, int y , int width, int height);
	static int moveAndStretchWidth( CWnd* parent, CWnd& widget, int x, int y, int border );
	static void moveAndStretch( CWnd* parent, CWnd& widget, int x, int y, int border );
	static void moveAndStretch( CWnd* parent, CWnd& widget, int x, int y, int xBorder, int yBorder );
	static void stretch( CWnd* parent, CWnd& widget, int border );

	static void stretchToRight( CWnd* parent, CWnd& widget, int pageWidth, int border );
	static void stretchToBottomRight( CWnd* parent, CWnd& widget, int pageWidth, int bx, int pageHeight, int by );

	static void moveToRight( CWnd* parent, CWnd& widget, int pageWidth, int border );
	static void moveToBottom( CWnd* parent, CWnd& widget, int pageHeight, int border );

	static void moveToTopRight( CWnd* parent, CWnd& widget, int pageWidth, int bx, int py );
	static void moveToBottomLeft( CWnd* parent, CWnd& widget, int pageHeight, int by, int px );

	static void centre( CWnd* parent, CWnd& widget, int pageWidth );

	static void fieldEnabledState( CEdit& field, bool enable, const BW::wstring& text = L"" );

	static BW::string pythonSafeName( const BW::string& s );
	static BW::wstring memorySizeToStr( uint32 memorySizeInBytes );

	static bool isOutdoorChunkReadyToProcess( Chunk* chunk, int xNeighs, int zNeighs, bool loadChunkIfNotReady );
	static bool isShellReadyToProcess( Chunk* chunk, int portalDepth, bool loadChunkIfNotReady );
	static bool isChunkReadyToProcess( Chunk* chunk, int depth, bool loadChunkIfNotReady );

	static void processMessagesForTask( bool* isEscPressed = NULL );

    static void compose
    (
        Matrix          &m,
        Vector3         const &scale,
        Vector3         const &trans,
        Vector3         const &rot
    );
    static void compose
    (
		Vector4         *axis0,
		Vector4         *axis1,
		Vector4         *axis2,
        Vector3         const &scale,
        Vector3         const &trans,
        Vector3         const &rot
    );
	static void decompose
	(
		Matrix          const &m,
		Vector3         &scale,
		Vector3         &trans,
		Vector3         &rot
	);
	static void decompose
	(
		Vector4         const &axis0,
		Vector4         const &axis1,
		Vector3         &scale,
		Vector3         &trans,
		Vector3         &rot
	);

	static bool captureWindow( CWnd& wnd, const BW::string& fileName );
};

BW_END_NAMESPACE

#endif // ULTILITIES_HPP
