#include "pch.hpp"
#include "utilities.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "controls/dib_section32.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

/*static*/ POINT Utilities::move( CWnd& widget, int x, int y )
{
	BW_GUARD;

	widget.SetWindowPos( NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	CRect rect;
	widget.GetWindowRect( &rect );
	POINT endOfWidget = {x + rect.Width(), y + rect.Height()};
	return endOfWidget;
}

/*static*/ void Utilities::moveAndSize( CWnd& widget, int x, int y , int width, int height)
{
	BW_GUARD;

	widget.SetWindowPos( NULL, x, y, width, height, SWP_NOZORDER );
}

/*static*/ int Utilities::moveAndStretchWidth( CWnd* parent, CWnd& widget, int x, int y, int border )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
	int height = rect.Height();

	parent->GetClientRect( &rect );
	int width = rect.Width() - border - x;

	widget.SetWindowPos( NULL, x, y, width, height, SWP_NOZORDER );

	return y + height;
}

/*static*/ void Utilities::moveAndStretch( CWnd* parent, CWnd& widget, int x, int y, int border )
{
	BW_GUARD;

	CRect rect;
	parent->GetClientRect( &rect );
	int width = rect.Width() - border - x;
	int height = rect.Height() - border - y;

	widget.SetWindowPos( NULL, x, y, width, height, SWP_NOZORDER );
}

/*static*/ void Utilities::moveAndStretch( CWnd* parent, CWnd& widget, int x, int y, int xBorder, int yBorder )
{
	BW_GUARD;

	CRect rect;
	parent->GetClientRect( &rect );
	int width = rect.Width() - xBorder - x;
	int height = rect.Height() - yBorder - y;

	widget.SetWindowPos( NULL, x, y, width, height, SWP_NOZORDER );
}

/*static*/ void Utilities::stretch( CWnd* parent, CWnd& widget, int border )
{
	BW_GUARD;

	CRect rect;
	parent->GetClientRect( &rect );

	widget.SetWindowPos( 0, border, border,
		rect.Width() - border * 2, rect.Height() - border * 2,
		SWP_NOZORDER );
}

/*static*/ void Utilities::stretchToRight( CWnd* parent, CWnd& widget, int pageWidth, int border )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    parent->ScreenToClient( &rect );
	widget.SetWindowPos( 0, rect.left, rect.top, pageWidth - rect.left - border, rect.Height(), SWP_NOZORDER );
}

/*static*/ void Utilities::stretchToBottomRight( CWnd* parent, CWnd& widget, int pageWidth, int bx, int pageHeight, int by )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    parent->ScreenToClient( &rect );
	widget.SetWindowPos( 0, rect.left, rect.top, pageWidth - rect.left - bx, pageHeight - rect.top - by, SWP_NOZORDER );
}

/*static*/ void Utilities::moveToRight( CWnd* parent, CWnd& widget, int pageWidth, int border )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    parent->ScreenToClient( &rect );
	int width = rect.right - rect.left;
	widget.SetWindowPos( 0, pageWidth - width - border , rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}

/*static*/ void Utilities::moveToBottom( CWnd* parent, CWnd& widget, int pageHeight, int border )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    parent->ScreenToClient( &rect );
	int height = rect.bottom - rect.top;
	widget.SetWindowPos( 0, rect.left, pageHeight - height - border, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}

/*static*/ void Utilities::moveToTopRight( CWnd* parent, CWnd& widget, int pageWidth, int bx, int py )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    parent->ScreenToClient( &rect );
	int width = rect.right - rect.left;
	widget.SetWindowPos( 0, pageWidth - bx , py, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}

/*static*/ void Utilities::moveToBottomLeft( CWnd* parent, CWnd& widget, int pageHeight, int by, int px )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    parent->ScreenToClient( &rect );
	int height = rect.bottom - rect.top;
	widget.SetWindowPos( 0, px , pageHeight - height - by, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}

/*static*/ void Utilities::centre( CWnd* parent, CWnd& widget, int pageWidth )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    parent->ScreenToClient( &rect );
	int width = rect.right - rect.left;
	widget.SetWindowPos( 0, pageWidth/2 - width/2 , rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}

/*static*/ void Utilities::fieldEnabledState( CEdit& field, bool enable, const BW::wstring& text )
{
	BW_GUARD;

	field.SetWindowText( text.c_str() );
	field.SetReadOnly( !enable );
	field.ModifyStyle( enable ? WS_DISABLED : 0, enable ? 0 : WS_DISABLED );
}

/*static*/ BW::string Utilities::pythonSafeName( const BW::string& s )
{
	BW_GUARD;

    BW::string r;

    bool isFirst = true;

	BW::string::const_iterator i = s.begin();
	for (; i != s.end(); ++i)
    {
        if (isFirst && isdigit( *i ))
		{
			r.push_back('_');
		}

		char c = tolower( *i );

        if ((c >= 'a' && c <= 'z') || isdigit( c ) || (c == '_'))
        {
			r.push_back( *i );
        }
        else if (c == ' ')
            r.push_back('_');

		isFirst = false;
    }

	return r;
}

/*static*/ BW::wstring Utilities::memorySizeToStr( uint32 memorySizeInBytes )
{
	BW_GUARD;

    BW::wstring result;
    int suffixIndex = 0;
    float memorySize = (float)memorySizeInBytes;

    while ( ( memorySize >= 1000.f ) && ( suffixIndex < 4 ))
    {
        memorySize /= 1024.f;
        suffixIndex++;
    }

    switch ( suffixIndex )
    {
        case 0:
            result = Localise(L"COMMON/UTILITIES/BYTES", Formatter( memorySize, L"%.1f") );
            break;
        case 1:
            result = Localise(L"COMMON/UTILITIES/KILOBYTES", Formatter( memorySize, L"%.1f") );
            break;
        case 2:
            result = Localise(L"COMMON/UTILITIES/MEGABYTES", Formatter( memorySize, L"%.1f") );
            break;
        case 3:
			result = Localise(L"COMMON/UTILITIES/GIGABYTES", Formatter( memorySize, L"%.1f") );
            break;
        case 4:
            result = Localise(L"COMMON/UTILITIES/TERABYTES", Formatter( memorySize, L"%.1f") );
            break;
    }

    return result;
}


/**
 *  This function check if the chunk is ready to process, i.e. it is bound
 *  and its neighbour chunk specified by the function parameters are also
 *	bound.
 *	It may also attempt to load chunk if any chunk is not bound
 *
 *  @param chunk    The given chunk to check
 *  @param xNeighs  How many outdoor chunk to load along x axis
 *  @param zNeighs  How many outdoor chunk to load along z axis
 *  @param loadNeighIfNotReady    Attempt to load a chunk if any chunk is not bound
 *  @return         a bool indicates whether the chunk is ready to process or not
 */
bool Utilities::isOutdoorChunkReadyToProcess( Chunk* chunk,
	int xNeighs, int zNeighs, bool loadChunkIfNotReady )
{
	MF_ASSERT( chunk->isOutsideChunk() );

	int16 gridX, gridZ;
	GeometryMapping* mapping = chunk->mapping();

	if (chunk->completed())
	{
		MF_VERIFY( mapping->gridFromChunkName( chunk->identifier(), gridX, gridZ ) );

		for (int z = gridZ - zNeighs; z <= gridZ + zNeighs; ++z)
		{
			for (int x = gridX - xNeighs; x <= gridX + xNeighs; ++x)
			{
				BW::string chunkName;
				chunkName = mapping->outsideChunkIdentifier( x, z, true );

				if (chunkName.empty())
				{// the neighbour chunk is out of bound
					continue;
				}

				if (Chunk* c = ChunkManager::instance().findChunkByName( chunkName, mapping ))
				{
					if (!c->completed())
					{
						return false;
					}
				}
			}
		}

		return false;
	}

	return true;
}


/**
 *  This function check if the chunk is ready to process, i.e. it is bound
 *  and its neighbour chunk specified by the function parameters are also
 *	bound.
 *	It may also attempt to load chunk if any chunk is not bound
 *
 *  @param chunk    The given chunk to check
 *  @param portalDepth    The portal traversal depth for shells
 *  @param loadNeighIfNotReady    Attempt to load a chunk if any chunk is not bound
 *  @return         a bool indicates whether the chunk is ready to process or not
 */
bool Utilities::isShellReadyToProcess( Chunk* chunk, int portalDepth, bool loadChunkIfNotReady )
{
	MF_ASSERT( !chunk->isOutsideChunk() );

	typedef BW::map<Chunk*, int> Chunks;

	Chunks visited;
	Chunks open;
	GeometryMapping* mapping = chunk->mapping();

	open[ chunk ] = 0;

	while (!open.empty())
	{
		int depth = open.begin()->second;

		chunk = open.begin()->first;
		visited[ chunk ] = depth;
		open.erase( open.begin() );

		if (!chunk->completed())
		{
			return false;
		}

		if (depth == portalDepth)
		{
			continue;
		}

		for (ChunkBoundaries::iterator bit = chunk->joints().begin();
			bit != chunk->joints().end(); ++bit)
		{
			for (ChunkBoundary::Portals::iterator ppit = (*bit)->unboundPortals_.begin();
				ppit != (*bit)->unboundPortals_.end(); ++ppit)
			{
				ChunkBoundary::Portal* pit = *ppit;

				if (pit->hasChunk())
				{// it has a chunk but unbound
					return false;
				}
			}
		}

		for (ChunkBoundaries::iterator bit = chunk->joints().begin();
			bit != chunk->joints().end(); ++bit)
		{
			for (ChunkBoundary::Portals::iterator ppit = (*bit)->boundPortals_.begin();
				ppit != (*bit)->boundPortals_.end(); ++ppit)
			{
				ChunkBoundary::Portal* pit = *ppit;

				if (pit->hasChunk())
				{
					if (visited.find( pit->pChunk ) == visited.end())
					{
						open[ pit->pChunk ] = depth + 1;
					}
				}
			}
		}
	}

	return true;
}


/**
 *  This function check if the chunk is ready to process, i.e. it is bound
 *  and its neighbour chunk specified by the function parameters are also
 *	bound.
 *	It may also attempt to load chunk if any chunk is not bound
 *
 *  @param chunk    The given chunk to check
 *  @param depth    The traversal depth
 *  @param loadNeighIfNotReady    Attempt to load a chunk if any chunk is not bound
 *  @return         a bool indicates whether the chunk is ready to process or not
 */
bool Utilities::isChunkReadyToProcess( Chunk* chunk, int depth, bool loadChunkIfNotReady )
{
	return chunk->isOutsideChunk()		?
		isOutdoorChunkReadyToProcess( chunk, depth, depth, loadChunkIfNotReady ) :
		isShellReadyToProcess( chunk, depth, loadChunkIfNotReady );
}


/**
 *  This function composes a matrix from scale, translation and 
 *  euler-angles (in degrees).
 *
 *  @param m        Return parameter that receives the composed matrix
 *  @param scale    Scale
 *  @param trans    Translation
 *  @param rot      Rotation (in degrees)
 */
void Utilities::compose
(
    Matrix          &m, 
    Vector3         const &scale, 
    Vector3         const &trans, 
    Vector3         const &rot
)
{
	BW_GUARD;

    Matrix rotm;
    rotm.setRotate(DEG_TO_RAD(rot.x), DEG_TO_RAD(rot.y), DEG_TO_RAD(rot.z));

    Matrix scalem;
    scalem.setScale(scale);

    Matrix transm;
    transm.setTranslate(trans);

    m = rotm;
    m.postMultiply(scalem);
    m.postMultiply(transm);
}


/**
 *  This function returns the axes of the coord system composed using the
 *  scale, translation and euler-angles (in degrees) passed in.
 *
 *  @param axis0    Return parameter for the first axis of the coord system
 *  @param axis1    Return parameter for the second axis of the coord system
 *  @param axis2    Return parameter for the third axis of the coord system
 *  @param scale    Scale
 *  @param trans    Translation
 *  @param rot      Rotation (in degrees)
 */
void Utilities::compose
(
	Vector4         *axis0, 
	Vector4         *axis1, 
	Vector4         *axis2, 
    Vector3         const &scale, 
    Vector3         const &trans, 
    Vector3         const &rot
)
{
	BW_GUARD;

	Matrix result;
	Utilities::compose(result, scale, trans, rot);

	if ( axis0 )
	{
		Vector3 vec = result.applyToUnitAxisVector(0);
		*axis0 = Vector4( vec.x, vec.y, vec.z, 0.0f );
	}
	if ( axis1 )
	{
		Vector3 vec = result.applyToUnitAxisVector(1);
		*axis1 = Vector4( vec.x, vec.y, vec.z, 0.0f );
	}
	if ( axis2 )
	{
		Vector3 vec = result.applyToUnitAxisVector(2);
		*axis2 = Vector4( vec.x, vec.y, vec.z, 0.0f );
	}
}

/**
 *  This function decomposes a matrix into scale, translation and
 *  euler-angles (in degrees).
 *
 *  @param m        Matrix to be decomposed
 *  @param scale    Receives the resulting scale vector
 *  @param trans    Receives the resulting translation vector
 *  @param rot      Receives the resulting rotation angles (in degrees)
 */
void Utilities::decompose
(
    Matrix          const &m, 
    Vector3         &scale, 
    Vector3         &trans, 
    Vector3         &rot
)
{
	BW_GUARD;

    Matrix matrix = m;

    // The translation:
    trans = matrix.applyToOrigin();
	matrix.translation(Vector3::zero());

    // The scale:
    scale.x = matrix.column(0).length();
    scale.y = matrix.column(1).length();
    scale.z = matrix.column(2).length();

    // Remove the effect of the scale to get the rotations
    if (scale.x != 0)
        matrix.column(0, matrix.column(0)*(1.0f/scale.x));
    if (scale.y != 0)									
        matrix.column(1, matrix.column(1)*(1.0f/scale.y));
    if (scale.z != 0)									
        matrix.column(2, matrix.column(2)*(1.0f/scale.z));

    // The rotation:
    rot.x = RAD_TO_DEG(matrix.yaw  ());
    rot.y = RAD_TO_DEG(matrix.pitch());
    rot.z = RAD_TO_DEG(matrix.roll ());
}


/**
 *  This function decomposes two non-parallel vectors into scale, translation
 *  and euler-angles (in degrees). It receives only two vectors, and the third
 *	vector of the coord system is calculated with their cross product.
 *
 *  @param axis0    First axis of the coord system to be decomposed
 *  @param axis1    Second axis of the coord system to be decomposed
 *  @param scale    Receives the resulting scale vector
 *  @param trans    Receives the resulting translation vector
 *  @param rot      Receives the resulting rotation angles (in degrees)
 */
void Utilities::decompose
(
    Vector4         const &axis0, 
    Vector4         const &axis1, 
    Vector3         &scale, 
    Vector3         &trans, 
    Vector3         &rot
)
{
	BW_GUARD;

    Vector3 u(axis0.x, axis0.y, axis0.z);
    Vector3 v(axis1.x, axis1.y, axis1.z);
    Vector3 w;
    w.crossProduct(v, u);
    Matrix xform(Matrix::identity);
    xform.column(0, axis0);
    xform.column(1, Vector4(w.x, w.y, w.z, 0.0f));
    xform.column(2, axis1);

	decompose(xform, scale, trans, rot);
}

/**
 * This function process messages in current message queue. But all mouse events,
 * keyboard events and menu events will be discarded.
 * This is used for preventing window from losing responding during some long time
 * calculation
 */
void Utilities::processMessagesForTask( bool* isEscPressed )
{
	BW_GUARD;

	MSG msg;
	while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
	{
		if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
		{
			if( msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE )
			{
				if ( isEscPressed != NULL )
				{
					(*isEscPressed) = true;
				}
			}
		}
		TranslateMessage( &msg );
		DispatchMessage(  &msg );
	}
}


/**
 *  This function captures the content of the window and saves it to the file.
 *
 *  @param wnd		Window to capture
 *  @param filename Name of the image file
 */
bool Utilities::captureWindow( CWnd& wnd, const BW::string& fileName )
{
	CRect rect;
	wnd.GetClientRect( &rect );
	controls::DibSection32 bitmap( rect.Width(), rect.Height() );
		
	CDC memDC;
	memDC.CreateCompatibleDC( wnd.GetDC() ); 

	HGDIOBJ oldbmp = memDC.SelectObject( bitmap );
	wnd.Print( &memDC, PRF_CHILDREN | PRF_CLIENT | PRF_ERASEBKGND );
	memDC.SelectObject( oldbmp );

	return bitmap.save( fileName );
}


BW_END_NAMESPACE
