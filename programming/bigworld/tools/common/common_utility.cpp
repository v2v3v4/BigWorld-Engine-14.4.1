//---------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
//---------------------------------------------------------------------------
#include "common_utility.h"
#include "moo/moo_dx.hpp"
#include "moo/moo_math.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/debug.hpp"

//---------------------------------------------------------------------------
#pragma package(smart_init)

DECLARE_DEBUG_COMPONENT2( "commonUtility", 2 );

//---------------------------------------------------------------------------
//using namespace CommonUtility;
//---------------------------------------------------------------------------
void __fastcall CommonUtility::displayHint(TObject *Sender)
{
	UNUSED_PARAMETER( Sender );
	//stsStatus->Panels->Items[0]->Text = GetLongHint(Application->Hint);
}
//---------------------------------------------------------------------------
float __fastcall CommonUtility::radClamp( float radian )
{
	while (radian > MATH_PI)
		radian -= MATH_2xPI;

	while (radian < -MATH_PI)
		radian += MATH_2xPI;

	return radian;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::getObjectName( unsigned int object )
{
	AnsiString num = "0000" + IntToStr( object );

    num = num.SubString( num.Length() - 5, 5 );

    return "object " + num;
}
//---------------------------------------------------------------------------
float __fastcall CommonUtility::strToFloatDef( AnsiString &str, float defaultValue )
{
    float retVal;

    try
    {
        retVal = (float)StrToFloat( str );        
    }
    catch ( EConvertError &exception )
    {
        retVal = defaultValue;
    }

    return retVal;
}
//---------------------------------------------------------------------------
TColor __fastcall CommonUtility::vectorToColor( Vector3 color )
{
	TColor tColor;

    (unsigned long)tColor = ( ((unsigned long)(color.x) | ((unsigned long)(color.y) << 8 ) | ((unsigned long)(color.z) << 16) ) );

    return tColor;
}
//---------------------------------------------------------------------------
TColor __fastcall CommonUtility::vectorToTColor( Vector3 color )
{
	TColor tColor;

    (unsigned long)tColor = ( ((unsigned long)(color.z) | ((unsigned long)(color.y) << 8 ) | ((unsigned long)(color.x) << 16) ) );

    return tColor;
}
//---------------------------------------------------------------------------
TColor __fastcall CommonUtility::mooColourToTColor( Moo::Colour colour )
{
	TColor tColour;

    int r = (colour.r * 255.f);
    int g = (colour.g * 255.f);
    int b = (colour.b * 255.f);

    tColour = r | g << 8 | b << 16;

    return tColour; 
}
//---------------------------------------------------------------------------
Moo::Colour __fastcall CommonUtility::TColorToMooColour( TColor colour )
{
	Vector4 c = Vector4( colorToVector( colour ), 255.f );
    c[0] /= 255.f;
    c[1] /= 255.f;
    c[2] /= 255.f;
    c[3] /= 255.f;
    //Moo::Colour col( c );

    Moo::Colour col;
    col.r = c[0];
    col.g = c[1];
    col.b = c[2];
    col.a = c[3];

    return col;
}
//---------------------------------------------------------------------------
Vector3 __fastcall CommonUtility::colorToVector( TColor color )
{
	Vector3 vColor;
	// convert the color to a Vector3
    vColor.x = (float)((color & 0x000000FF)       );   // red
    vColor.y = (float)((color & 0x0000FF00) >>  8 );   // green
    vColor.z = (float)((color & 0x00FF0000) >> 16 );   // blue

	return vColor;
}
//---------------------------------------------------------------------------
Vector3 __fastcall CommonUtility::TColorToVector( TColor color )
{
	Vector3 vColor;
	// convert the color to a Vector3
    vColor.x = (float)((color & 0x00FF0000) >> 16 );   // red
    vColor.y = (float)((color & 0x0000FF00) >>  8 );   // green
    vColor.z = (float)((color & 0x000000FF)       );   // blue

	return vColor;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::getFile( AnsiString filename )
{
	return filename.SubString( filename.LastDelimiter( "\\/" ) + 1, filename.Length() ).LowerCase();
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::getDirectory( AnsiString filename )
{
	return filename.SubString( 1, filename.LastDelimiter( "\\/" ) );
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::getExtension( AnsiString filename )
{
	int index = filename.LastDelimiter( "." ) + 1;
    int length = filename.Length();
	return filename.SubString( index, length-index+1 ).LowerCase();
}
//---------------------------------------------------------------------------
void __fastcall CommonUtility::gatherExtensions( AnsiString filename,
		BW::vector<BW::string>& results )
{
	AnsiString f = filename;
    int dotIndex = f.LastDelimiter( "." ) + 1;
    int slashIndex = f.LastDelimiter( "/" ) + 1;
    int backSlashIndex = f.LastDelimiter( "\\" ) + 1;

    while ( dotIndex > slashIndex &&
    		dotIndex > backSlashIndex &&
            dotIndex > 0 )
    {
    	int length = f.Length();
    	results.push_back(
        	ANSI_TO_STD( f.SubString( dotIndex, length-dotIndex+1 ).LowerCase() ) );
        f = filename.SubString( 1, dotIndex-2 );

        dotIndex = f.LastDelimiter( "." ) + 1;
        slashIndex = f.LastDelimiter( "/" ) + 1;
        backSlashIndex = f.LastDelimiter( "\\" ) + 1;
    }
}
//---------------------------------------------------------------------------
int __fastcall CommonUtility::snapToNearest( int snap, int nearest )
{
	int value = snap;

	if ( nearest != 0 )
    {
    	int halfNearest = nearest >> 1;

		if ( value >= 0 )
    	{
	    	if ( value % nearest < halfNearest )
    	    	value -= value % nearest;
        	else
        		value += nearest - ( value % nearest );
	    }
    	else
	    {
    		if ( -value % nearest < halfNearest )
	        	value += -value % nearest;
    	    else
        		value -= nearest - ( -value % nearest );
	    }
    }

    return value;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::removePath( AnsiString filename )
{
	AnsiString name;
    int start, backSlashPos, forewardSlashPos, end;

    backSlashPos = filename.LastDelimiter( "\\" ) + 1;
    forewardSlashPos = filename.LastDelimiter( "/" ) + 1;

    if ( backSlashPos > forewardSlashPos )
    	start = backSlashPos;
    else
    	start = forewardSlashPos;

    end = filename.Length() + 1;

    if ( start < end )
    {
		for ( int i = start; i < end; i++ )
    		name += filename[i];
    }
    else
    	name = filename;

    return name;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::removeExtension( AnsiString filename )
{
	AnsiString file;
	int dot = filename.LastDelimiter( "." );

    if ( dot )
    	file = filename.SubString( 1, dot - 1 );
    else
    	file = filename;

    return file;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::removeExtensions( AnsiString filename )
{
	bool finished = false;
    AnsiString file = filename;

    while (!finished )
    {
        int dot = file.LastDelimiter( "." );

        if ( dot > 0 )
            file = file.SubString( 1, dot - 1 );
        else
        {
            finished = true;
        }
    }

    return file;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::vectorToString( Vector3 v, AnsiString seperator )
{
	AnsiString s;

    AnsiString v0 = FormatFloat( "##########0.0##", v[0] );
    AnsiString v1 = FormatFloat( "##########0.0##", v[1] );
    AnsiString v2 = FormatFloat( "##########0.0##", v[2] );

    s = v0 + seperator + v1 + seperator + v2;

    return s;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::vectorToString( Vector4 v, AnsiString seperator, bool asFloats )
{
    AnsiString format = "##########0.0##";
    if (!asFloats)
        format = "##########0";

	AnsiString s;

    AnsiString v0 = FormatFloat( format, v[0] );
    AnsiString v1 = FormatFloat( format, v[1] );
    AnsiString v2 = FormatFloat( format, v[2] );
    AnsiString v3 = FormatFloat( format, v[3] );

    s = v0 + seperator + v1 + seperator + v2 + seperator + v3;

    return s;

}
//---------------------------------------------------------------------------
bool __fastcall CommonUtility::stringToVector( AnsiString s, Vector3& v )
{
	// Remove spaces and other unauthorized characters
    AnsiString t = "";

    for ( int i = 1; i <= s.Length(); i++ )
    {
    	// only allow valid float characters ( no scientific format support - (e or E ) )
		if ( s.IsDelimiter( "+-0123456789,.", i ) )
        	t += s[i];
    }

    AnsiString token;
    int start = 1;
    int tokens = 0;

    while ( ( tokens < 3 ) && getToken( t, start, ",", token ) )
    {
    	if ( token != "" )
        {
        	// convert the token to a float and put it in the right spot in the vector
	        try
    	    {
				float ft = StrToFloat( token );

                v[tokens] = ft;
	        }
    	    catch(...)
        	{
				return false;
    	    }
        }

        start += token.Length() + 1;
        tokens++;
	}

    return true;
}
//---------------------------------------------------------------------------
bool __fastcall CommonUtility::stringToVector( AnsiString s, Vector4& v )
{
	// Remove spaces and other unauthorized characters
    AnsiString t = "";

    for ( int i = 1; i <= s.Length(); i++ )
    {
    	// only allow valid float characters ( no scientific format support - (e or E ) )
		if ( s.IsDelimiter( "+-0123456789,.", i ) )
        	t += s[i];
    }

    AnsiString token;
    int start = 1;
    int tokens = 0;

    while ( ( tokens < 4 ) && getToken( t, start, ",", token ) )
    {
    	if ( token != "" )
        {
        	// convert the token to a float and put it in the right spot in the vector
	        try
    	    {
				float ft = StrToFloat( token );

                v[tokens] = ft;
	        }
    	    catch(...)
        	{
				return false;
    	    }
        }

        start += token.Length() + 1;
        tokens++;
	}

    return true;
}
//---------------------------------------------------------------------------
bool __fastcall CommonUtility::getToken( AnsiString s, int start, AnsiString delimiters, AnsiString& token )
{
	AnsiString t;

    token = "";

    if ( start <= s.Length() )
    {
	    for ( int i = start; i <= s.Length(); i++ )
    	{
    		if ( !s.IsDelimiter( delimiters, i ) )
        		t += s[i];
            else
            	break;
	    }

		token = t;

		return true;
    }
    return false;
}
//---------------------------------------------------------------------------
void __fastcall CommonUtility::doubleBufferControl( TWinControl* control )
{
	control->DoubleBuffered = true;

    for ( int i = 0; i < control->ControlCount; i++ )
    {
		TControl* ctrl = control->Controls[i];

        AnsiString name = ctrl->ClassName();

        //if ( ctrl->InheritsFrom( TWinControl::ClassType() ) )
        if ( ctrl->ClassNameIs( "TPanel" )           ||
             ctrl->ClassNameIs( "TToolbar" )         ||
             ctrl->ClassNameIs( "TGroupBox" )	     ||
             ctrl->ClassNameIs( "TPageControl ")	 ||
             ctrl->ClassNameIs( "TTabSheet ")	     ||
        	 ctrl->ClassNameIs( "TShadePanel" ) )
        {
        	CommonUtility::doubleBufferControl( (TWinControl*)ctrl );
        }

        if ( ctrl->ClassNameIs( "TEdit" )            ||
        	 ctrl->ClassNameIs( "TUpDown" )          ||
        	 ctrl->ClassNameIs( "TFlatEdit" )        ||
             ctrl->ClassNameIs( "TFlatButton" )      ||
             ctrl->ClassNameIs( "TFlatCheckBox" )    ||
             ctrl->ClassNameIs( "TFlatRadioButton" ) ||
             ctrl->ClassNameIs( "TFlatSpeedButton" ) )
        {
        	((TWinControl*)ctrl)->DoubleBuffered = true;
        }
    }
}
//---------------------------------------------------------------------------
bool __fastcall CommonUtility::isMiddle( bool bUseMiddle, TShiftState Shift )
{
	if ( bUseMiddle )
		return Shift.Contains( ssMiddle );

    return Shift.Contains( ssAlt ) && Shift.Contains( ssLeft );
}
//---------------------------------------------------------------------------
bool __fastcall CommonUtility::isMiddle( bool bUseMiddle, TShiftState Shift, TMouseButton Button )
{
	if ( bUseMiddle )
		return Button == mbMiddle;

    return Shift.Contains( ssAlt ) && Button == mbLeft;
}
//---------------------------------------------------------------------------
//ripped from directX Textr.cpp
//modified to slpit the image / alpha channel and store in separate TImage
//controls
/*void __fastcall CommonUtility::loadTGAImage( AnsiString filename, Graphics::TBitmap* rgbDest, Graphics::TBitmap* alphaDest )
{
#pragma pack ( push, 1 )
	struct TGAHEADER {
		unsigned char  idLength;
		unsigned char  colorMapType;
		unsigned char  imageType;
		unsigned char  colorMapSpec[5];
		unsigned short imgXorigin;
		unsigned short imgYorigin;
		unsigned short imgWidth;
		unsigned short imgHeight;
		unsigned char  imgDepth;
		unsigned char  imgDescriptor;
	} tgaHeader;

	struct TGAFOOTER {
		unsigned long extensionOffset;
		unsigned long DeveloperOffset;
				 char signature[16];
		unsigned char dot;
		unsigned char zero;
	} tgaFooter;

    //int tgaHeaderSize = sizeof( tgaHeader );
    int tgaFooterSize = sizeof( tgaFooter );
#pragma pack ( pop )

	const char TGA_SIGNATURE[] = "TRUEVISION-XFILE";

	FILE *f;

	if ( ( f = bw_fopen( filename.c_str(), "rb" ) ) != NULL )
	{
		fseek( f, -tgaFooterSize, SEEK_END );


		// read the last 16 bytes of the file and check its signature
		fread( &tgaFooter, 1, tgaFooterSize, f );

		if ( strncmp( tgaFooter.signature, TGA_SIGNATURE, 16 ) == 0 )
		{
			// file is a valid TGA,
			// load the TGA file header
			fseek( f, 0, SEEK_SET );
			fread( &tgaHeader, 1, 18, f );

			// is it a valid file type - no color map, and image is uncompressed true-color
			if ( tgaHeader.idLength == 0 && tgaHeader.colorMapType == 0 && tgaHeader.imageType == 2 )
			{
				// check the width and height
                bool is32Bit = ( tgaHeader.imgDepth == 32 );

				if ( tgaHeader.imgDepth == 24 || tgaHeader.imgDepth == 32 )
				{
                    //make sure the destination images have the right size
                    //and use 32bits.
					rgbDest->Width = tgaHeader.imgWidth;
					rgbDest->Height = tgaHeader.imgHeight;

                    if ( is32Bit )
	                    rgbDest->PixelFormat = pf32bit;
                    else
                        rgbDest->PixelFormat = pf24bit;

                    if ( is32Bit && alphaDest )
                    {
	                    alphaDest->Width = tgaHeader.imgWidth;
	                    alphaDest->Height = tgaHeader.imgHeight;
	                    alphaDest->PixelFormat = pf32bit;
                    }

                    //read scanlines into a scanline-length buffer
                    uint32 *pixels = new uint32[ tgaHeader.imgWidth * 4 ];

					if ( pixels )
					{
						for( uint32 y = 0; y < tgaHeader.imgHeight; y++ )
						{
                        	if ( is32Bit )
								fread( pixels , 4, tgaHeader.imgWidth, f );
                            else
								fread( pixels , 3, tgaHeader.imgWidth, f );

                            //now write the current scanline out to the image control
                            //remembering to reverse the image in the y-direction

                            //RGB
                            if ( is32Bit )
                            {
	                            uint32 *rgbScanline = (uint32 *)rgbDest->ScanLine[tgaHeader.imgHeight-1-y];
    	                        memcpy( rgbScanline, pixels, tgaHeader.imgWidth * 4 );
							}
                            else
                            {
	                            uint32 *rgbScanline = (uint32 *)rgbDest->ScanLine[tgaHeader.imgHeight-1-y];
    	                        memcpy( rgbScanline, pixels, tgaHeader.imgWidth * 3 );
                            }

                            //Alpha
                            if ( is32Bit && alphaDest )
                            {
	                            uint32 *alphaScanline = (uint32 *)alphaDest->ScanLine[tgaHeader.imgHeight-1-y];

    	                        for( uint32 x = 0; x < tgaHeader.imgWidth; x++)
        	                    {
            	                    Byte a = (Byte)((pixels[x] & 0xff000000) >> 24);
                	                uint32 a32 = (a << 16) | (a << 8) | a;
                    	            *alphaScanline++ = a32;
                        	    }
                            }
						}

                        delete[] pixels;
					}
				}
			}
		}
		fclose( f );
	}
}    */
//---------------------------------------------------------------------------
int __fastcall CommonUtility::countFiles( AnsiString root, int attr, bool includeSubDirectories )
{
	int count = 0;
    TSearchRec sr;

    AnsiString path = root + "/*.*";

    if ( FindFirst( path, attr, sr ) == 0 )
    {
    	int a;
        bool pass;

        do
        {
        	pass = false;

        	pass |= ( ( attr == faDirectory ) && ( ( sr.Attr & faDirectory) == faDirectory ) );
            pass |= ( ( attr == 0x3F ) );

            if ( pass  )
            {
            	if ( sr.Name != "." && sr.Name != ".." )
	            {
    	        	count ++;

            		if ( includeSubDirectories )
                		count += countFiles( root + "/" + sr.Name, attr, includeSubDirectories );
	            }
            }
        } while ( FindNext( sr ) == 0 );

        FindClose(sr);
    }

	return count;
}
//---------------------------------------------------------------------------
void __fastcall CommonUtility::cameraLookAt( Matrix34& matrix, Vector3& location )
{
    Matrix34 m = matrix;
    m.invertOrthonormal( );
	Vector3 camLocation = m[3];
	matrix.lookAt( camLocation, location - camLocation, Vector3( 0, 1, 0 ) );
}
//---------------------------------------------------------------------------
void __fastcall CommonUtility::zoomExtents( float fov, Matrix34& camera, BoundingBox& bbox )
{
    float radius;

    //Calculate the world-space center of the mesh object, based on
    //the center of the local min/max bounding box metrics and the
    //meshes current position
    Vector3 max = bbox.maxBounds( );
    Vector3 min = bbox.minBounds( );
    Vector3 maxMinusMinOnTwo = ( max - min ) / 2.f;
    Vector3 objectCenter = min + maxMinusMinOnTwo;

    //Calculate the relative camera position.  This is based on the current
    //relative position to the center of the mesh
    camera.invertOrthonormal();
    Vector3 relativePosition = camera[3] - objectCenter;
    relativePosition.normalise( );

    //transfrom the bounding box into camera space
    bbox.transformBy( camera );

    //Calculate the radius of the bounding box, as seen by the camera
    Vector3 maxBounds( bbox.maxBounds( ) );
    maxBounds -= bbox.minBounds( );
    //radius = maxBounds.length( ) / 2.f;
    float radX = ( bbox.maxBounds( )[0] - bbox.minBounds( )[0] );
    float radY = ( bbox.maxBounds( )[1] - bbox.minBounds( )[1] );

    radius = ( radX > radY) ? ( radX / 2.f ) : ( radY / 2.f );

    //Now calculate the distance the camera must be from the object.
    //We calculate this using dist = ( objectWidth/2 ) / tan( FOV/2 );
    float dist = radius / tan( fov / 2.f );

    relativePosition *= ( dist * 1.1 );
    relativePosition += objectCenter;

    camera.setTranslate( -relativePosition.x, -relativePosition.y, -relativePosition.z );
    camera.lookAt( relativePosition, objectCenter - relativePosition, Vector3( 0, 1, 0 ) );
}
//---------------------------------------------------------------------------
//
//  Formats an amount of memory, as a string
//
//
//  e.g. 400 = 400bytes
//      1025 = 1KB
//  20493000 = 2MB
//
AnsiString __fastcall CommonUtility::memorySizeToStr( uint32 memorySizeInBytes )
{
    AnsiString result;
    int suffixIndex = 0;
    float memorySize = (float)memorySizeInBytes;

    while ( ( memorySize > 1000.f ) && ( suffixIndex < 4 ))
    {
        memorySize /= 1024.f;
        suffixIndex++;
    }

    result = FloatToStrF( memorySize, ffNumber, 12, 2 );

    switch ( suffixIndex )
    {
        case 0:
            result += " Bytes";
            break;
        case 1:
            result += " KB";
            break;
        case 2:
            result += " MB";
            break;
        case 3:
            result += " GB";
            break;
        case 4:
            result += " TB";
            break;
    }

    return result;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::bestFitLabel( TCanvas* Canvas, AnsiString caption, int width )
{
	AnsiString label = caption;

    // make sure the label fits in the allocated space for the label
    if ( Canvas->TextWidth( label ) > width )
    {
    	for ( int i = label.Length(); i; i-- )
        {
			if ( caption[i] == '\\' || caption[i] == '/' )
            {
            	// get a new label to test the fit of
            	AnsiString test = "..." + caption.SubString( i, caption.Length() );

                if ( Canvas->TextWidth( test ) > width )
                	// the current label is the closest fit
                	break;

				// best fit so far, but can we do better?
                label = test;
            }
        }
    }

    return label;
}
//---------------------------------------------------------------------------
TMouseButton __fastcall CommonUtility::shiftToMouse( TShiftState shift )
{
	TMouseButton button;

    if ( shift.Contains( ssLeft ) )
    	button = mbLeft;
    else
    if ( shift.Contains( ssRight ) )
    	button = mbRight;
    else
    if ( shift.Contains( ssMiddle ) )
    	button = mbMiddle;

    return button;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::convertPosixToWindows( const AnsiString& posixDirectory )
{
	AnsiString winDirectory = posixDirectory;

    for ( int i = 1; i <= winDirectory.Length( ); i++ )
    {
    	if ( winDirectory[i] == '/' )
        	winDirectory[i] = '\\';
    }

    return winDirectory;
}
//---------------------------------------------------------------------------
AnsiString __fastcall CommonUtility::convertWindowsToPosix( const AnsiString& winDirectory )
{
	AnsiString posixDirectory = winDirectory;

    for ( int i = 1; i <= posixDirectory.Length( ); i++ )
    {
    	if ( posixDirectory[i] == '\\' )
        	posixDirectory[i] = '/';
    }

    return posixDirectory;
}


//---------------------------------------------------------------------------
/**
 * 	This method loads a DDS image into the given Bitmap controls.
 *
 */

/*void __fastcall CommonUtility::loadDDSImage(
		AnsiString filename,
        Graphics::TBitmap* rgbDest,
        Graphics::TBitmap* alphaDest )
{
    AnsiString tganame = ChangeFileExt( filename, ".tga" );
    AnsiString bmpname = ChangeFileExt( filename, ".bmp" );
    if (FileExists( tganame ))
    {
        CommonUtility::loadTGAImage(
                tganame,
                rgbDest,
                alphaDest );

    }
    else if (FileExists( bmpname ))
    {
        rgbDest->LoadFromFile( bmpname );
    }
}    */


/**
 *	This method loads any image.
 *
 *	It uses DX to do all the hard work; in this way we can support any
 *	DX format image on a TBitmap
 *
 *	Note : you must have a valid Moo::rc().device(), and the hardware
 *	you are running on must support the dds' compression mode etc.
 */
void __fastcall CommonUtility::loadAnyImage(
	AnsiString filename,
    Graphics::TBitmap* rgbDest,
    Graphics::TBitmap* alphaDest )
{
    if (filename == "")
        return;

    // Removed this debug message since it was flooding the message window in Modelviewer
    //DEBUG_MSG( "LoadAnyImage : %s\n", filename.c_str() );

	if ( !Moo::rc().device() )
    {
    	MF_ASSERT( "Cannot loadDDSImage without a valid device" );
        return;
    }

    ComObjectWrap<DX::Texture> pTexture;

    try
    {
    	//load dds into a texture
        pTexture = Moo::rc().createTextureFromFileEx(
            filename.c_str(),
            rgbDest->Width,
            rgbDest->Height,
            1,
            0,
            D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM,
            D3DX_DEFAULT,
            D3DX_DEFAULT,
            0xFF000000,
            NULL );
    }
    catch(...)
    {
        WARNING_MSG( "CommonUtility::loadAnyImage - Unknown error\n" );
        return;
    }

    if ( !pTexture )
    {
        WARNING_MSG( "CommonUtility::loadDDSImage - Could not create texture from file %s\n", filename.c_str() );
    }
    else
    {
        try
        {
        //DEBUG_MSG( "CommonUtility::loadDDSImage - lockRect" );
    	//now, lock the texture and copy the pixels to the destination images.
        D3DLOCKED_RECT lockedRect;
        ZeroMemory( &lockedRect, sizeof( lockedRect ) );
        HRESULT hr = pTexture->LockRect( 0, &lockedRect, NULL, D3DLOCK_READONLY );
        if ( SUCCEEDED( hr ) )
        {
        	//Setup the destination bitmaps
            rgbDest->PixelFormat = pf32bit;
            if (alphaDest)
                alphaDest->PixelFormat = pf32bit;

			//Loop through each scanline
            int width = rgbDest->Width;
            int height = rgbDest->Height;

            // Removed this debug message since it was flooding the message window in Modelviewer
            //DEBUG_MSG( "CommonUtility::loadDDSImage - about to loop" ); 
            for ( int y = 0; y < height; y++ )
            {
            	//grab a pointer to the current scanline
                char* pBytes = (char*)lockedRect.pBits;
                uint32 *pixels = (uint32*)( pBytes + y * lockedRect.Pitch );

            	//copy RGB
                uint32 *rgbScanline = (uint32 *)rgbDest->ScanLine[y];
                memcpy( rgbScanline, pixels, width * 4 );

                //copy Alpha
                if ( alphaDest )
                {
                    uint32 *alphaScanline = (uint32 *)alphaDest->ScanLine[y];

                    for( uint32 x = 0; x < width; x++)
                    {
                        Byte a = (Byte)((pixels[x] & 0xff000000) >> 24);
                        uint32 a32 = (a << 16) | (a << 8) | a;
                        *alphaScanline++ = a32;
                    }
                }
            }

            pTexture->UnlockRect( 0 );
        }
        else
        {
        	WARNING_MSG( "CommonUtility::loadDDSImage - Could not lock texture resource %s\n", filename.c_str() );
        }

        //DEBUG_MSG( "LoadAnyImage : OK\n" );
        }
         catch(...)
        {
            WARNING_MSG( "CommonUtility::loadAnyImage - Unknown error copying texture memory\n" );
            return;
        }
    }
}

