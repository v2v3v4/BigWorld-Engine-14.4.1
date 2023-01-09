//---------------------------------------------------------------------------
#ifndef common_utilityH
#define common_utilityH
//- VCL ---------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <ExtCtrls.hpp>
//- Math -------------------------------------------------------------------
#include "math/vector3.hpp"
#include "math/matrix.hpp"
#include "math/boundbox.hpp"
//moo
#include "moo/moo_math.hpp"
//stl
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
//---------------------------------------------------------------------------
#define SIGN(a) ( a >= 0 ? 1 : -1 )
#define ANSI_TO_STD( a ) ( BW::string( a.c_str() ) )
#define STD_TO_ANSI( a ) ( AnsiString( a.c_str() ) )
#define UNUSED_PARAMETER( a ) ( a )
#ifndef MATH_PI
#define MATH_PI (3.14159265359f)
#endif
#define MATH_2xPI (6.28318530718f)
#define DEG_TO_RAD(angle) ((float)(angle) * MATH_PI / 180.f)
#define RAD_TO_DEG(angle) ((float)(angle) * 180.f / MATH_PI)
//---------------------------------------------------------------------------
class CommonUtility
{
public:
    static void __fastcall displayHint(TObject *Sender);
    static float __fastcall radClamp( float radian );
    static AnsiString __fastcall getObjectName( unsigned int object );
    static float __fastcall strToFloatDef( AnsiString &str, float defaultValue );
    static TColor __fastcall vectorToColor( Vector3 colour );
	static TColor __fastcall vectorToTColor( Vector3 colour );
    static TColor __fastcall mooColourToTColor( Moo::Colour colour );
    static Moo::Colour __fastcall TColorToMooColour( TColor colour );
    static Vector3 __fastcall colorToVector( TColor colour ); 	// rhino rgb colors
	static Vector3 __fastcall TColorToVector( TColor colour );	// vcl bgr colors
    static AnsiString __fastcall getFile( AnsiString filename );
    static AnsiString __fastcall getDirectory( AnsiString filename );
    static AnsiString __fastcall getExtension( AnsiString filename );
    static void __fastcall gatherExtensions( AnsiString filename,
			BW::vector<BW::string>& results );
    static int __fastcall snapToNearest( int snap, int nearest );
    static AnsiString __fastcall removePath( AnsiString filename );
    static AnsiString __fastcall removeExtension( AnsiString filename );
    static AnsiString __fastcall removeExtensions( AnsiString filename );
    static AnsiString __fastcall vectorToString( Vector3 v, AnsiString seperator );
    static AnsiString __fastcall vectorToString( Vector4 v, AnsiString seperator, bool asFloats = true );
    static bool __fastcall stringToVector( AnsiString s, Vector3& v );
    static bool __fastcall stringToVector( AnsiString s, Vector4& v );
    static bool __fastcall getToken( AnsiString s, int start, AnsiString delimiters, AnsiString& token );
    static void __fastcall doubleBufferControl( TWinControl* control );
    static bool __fastcall isMiddle( bool bUseMiddle, TShiftState Shift );
    static bool __fastcall isMiddle( bool bUseMiddle, TShiftState Shift, TMouseButton Button );
//	static void __fastcall loadTGAImage( AnsiString filename, Graphics::TBitmap* rgbDest, Graphics::TBitmap* alphaDest );
//    static void __fastcall loadDDSImage( AnsiString filename, Graphics::TBitmap* rgbDest, Graphics::TBitmap* alphaDest );
	static void __fastcall loadAnyImage( AnsiString filename, Graphics::TBitmap* rgbDest, Graphics::TBitmap* alphaDest );
    //static void __fastcall loadTGAImage( char *filename, TImage &rgbDest, TImage &alphaDest );
	static void __fastcall cameraLookAt( Matrix34& matrix, Vector3& location );
	static int  __fastcall countFiles( AnsiString root, int attr, bool includeSubDirectories );
	static void __fastcall zoomExtents( float fov, Matrix& camera, BoundingBox& bbox );
    static AnsiString __fastcall memorySizeToStr( uint32 memorySizeInBytes );
	static AnsiString _fastcall bestFitLabel( TCanvas* Canvas, AnsiString caption, int width );
	static TMouseButton __fastcall shiftToMouse( TShiftState shift );
	static AnsiString __fastcall convertPosixToWindows( const AnsiString& posixDirectory );
	static AnsiString __fastcall convertWindowsToPosix( const AnsiString& winDirectory );
	static bool __fastcall canRun( void );
};
//---------------------------------------------------------------------------
#endif
