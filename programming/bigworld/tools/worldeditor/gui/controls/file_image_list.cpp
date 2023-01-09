#include "pch.hpp"
#include "atlimage.h"
#include "file_image_list.hpp"
#include "commctrl.h"

#include "moo/moo_dx.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_lock_wrapper.hpp"

BW_BEGIN_NAMESPACE

// FileImageList

IMPLEMENT_DYNAMIC(FileImageList, CListCtrl)
FileImageList::FileImageList()
: directoryMask_( "" )
, callbackFn_( NULL )
{
}

FileImageList::~FileImageList()
{
	BW_GUARD;

	imageList_.DeleteImageList();
}

void FileImageList::initialise() 
{
	BW_GUARD;

	// create an empty 64x64 image list
	const int cx = 64;
	const int cy = 64;
	const int initialImageCount = 0;
	const int maximumImageCount = 1000;
	imageList_.Create(cx, cy, ILC_COLOR24, initialImageCount, maximumImageCount);

	// push on the system images
	SHFILEINFO lsfi;
	HIMAGELIST hSystemLargeImageList = 
		(HIMAGELIST)SHGetFileInfo( 
		(LPCTSTR)_T("C:\\"), 
		0, 
		&lsfi, 
		sizeof(SHFILEINFO), 
		SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_LARGEICON | SHGFI_SHELLICONSIZE); 

#if 0
	// TODO: add the system images
	CImageList tempList;
	tempList.Attach(hSystemLargeImageList);
	int count = tempList.GetImageCount();
	IMAGEINFO imageInfo;
	for (int i = 0; i < count; i++)
	{
		const BOOL ok = tempList.GetImageInfo(i, &imageInfo);
		ASSERT(ok);
		AddImage(CBitmap::FromHandle(imageInfo.hbmImage));
	}
	tempList.Detach();
#endif

	//Set the list control image list 
	SetImageList(&imageList_, LVSIL_NORMAL); 

	// draw the icons a little closer together
	CSize theSize = SetIconSpacing(100, 100);
}

static const wchar_t * THUMB_NAIL_SUFFIX = L".thumbnail.bmp";
int FileImageList::addImage(const CString& fullFilenameOriginal )
{
	BW_GUARD;

	bool saveThumbnail = true;
	CString fullFilename = fullFilenameOriginal;
	if( BWResource::fileExists( bw_wtoutf8( ( fullFilename + THUMB_NAIL_SUFFIX ).GetString() ) ) )
	{
		HANDLE img = CreateFile( fullFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
		HANDLE imgThumb = CreateFile( fullFilename + THUMB_NAIL_SUFFIX, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
		if( img != INVALID_HANDLE_VALUE && imgThumb != INVALID_HANDLE_VALUE )
		{
			FILETIME imgTime, imgThumbTime;
			GetFileTime( img, NULL, NULL, &imgTime );
			GetFileTime( imgThumb, NULL, NULL, &imgThumbTime );
			if( *(__int64*)&imgTime < *(__int64*)&imgThumbTime )
			{
				fullFilename += THUMB_NAIL_SUFFIX;
				saveThumbnail = false;
			}
		}
		if( img != INVALID_HANDLE_VALUE )
			CloseHandle( img );
		if( imgThumb != INVALID_HANDLE_VALUE )
			CloseHandle( imgThumb );
	}
	if( saveThumbnail )
	{
		static const DWORD MIN_THUMBNAIL_FILE_SIZE = 200 * 1024;//200K
		WIN32_FIND_DATA findData;
		HANDLE find = FindFirstFile( fullFilename, &findData );
		if( find != INVALID_HANDLE_VALUE )
		{
			if( findData.nFileSizeLow <= MIN_THUMBNAIL_FILE_SIZE )
				saveThumbnail = false;
			FindClose( find );
		}
	}

	int imageIndex = -1;

	if( !saveThumbnail )
	{
		CImage theImage;
		HRESULT hr = theImage.Load( fullFilename );
		if (SUCCEEDED(hr))
			imageIndex = addImage( CBitmap::FromHandle( theImage ) );
	}

	if( imageIndex == -1 )
	{
		ComObjectWrap<DX::Texture> pTexture;

		//load dds into a texture
		pTexture = Moo::rc().createTextureFromFileEx(
            fullFilename,
            64,
            64,
            1,
            0,
            D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM,
            D3DX_DEFAULT,
            D3DX_DEFAULT,
            0xFF000000,
            NULL,
            NULL );

		if ( pTexture )
		{
			D3DLOCKED_RECT lockedRect = { sizeof(D3DLOCKED_RECT) };
			Moo::TextureLockWrapper textureLock( pTexture );

			HRESULT hr = textureLock.lockRect( 0, &lockedRect, NULL, D3DLOCK_READONLY );
			if ( SUCCEEDED( hr ) )
			{
				//grab a pointer to the current scanline
				const char* pBytes = (const char*)lockedRect.pBits;        
				CBitmap* bmp = new CBitmap;			
				bmp->CreateBitmap(64,64,1,32,pBytes);
				imageIndex = addImage(bmp);
				textureLock.unlockRect( 0 );				
				bw_safe_delete(bmp);
			}
			if( saveThumbnail )
				D3DXSaveTextureToFile( fullFilename + THUMB_NAIL_SUFFIX, D3DXIFF_BMP, pTexture.pComObject(), NULL );
		}
	}

	imageFileMap_.insert(std::pair<CString, int>(fullFilename, imageIndex));
	return imageIndex;
}

int FileImageList::addImage(CBitmap * bmpImage)
{
	BW_GUARD;

	int pos = -1;

#if 0 // TODO: add system icons
	BITMAP bmpInfo;
	bmpImage->GetBitmap(&bmpInfo);
	if (bmpInfo.bmWidth != 64 || bmpInfo.bmHeight != 64)
	{
		// massage it!
	}
	else
        pos = imageList_.Add(bmpImage, (CBitmap*)0);

	return pos;
#endif

	return imageList_.Add(bmpImage, (CBitmap*)0);
}

int FileImageList::findImageIndex(const CString& fullFilename)
{
	BW_GUARD;

	// check to see if custom image
	CString fullImageFilename = fullFilename;
	for (StringStringMap::iterator i = fileImageRepMap_.begin();
		i != fileImageRepMap_.end();
		i++)
	{
		CString ext(i->first);
		CString rep(i->second);
		const int extLength = ext.GetLength();
		if (fullFilename.Right(extLength) == ext)
		{
			fullImageFilename = fullFilename.Left(fullFilename.GetLength() - extLength);
			fullImageFilename += rep;
			break;
		}
	}

	StringIntMap::iterator it = imageFileMap_.find(fullImageFilename);
	if (it != imageFileMap_.end())
	{
		return it->second;
	}

	return addImage(fullImageFilename);
}

void FileImageList::fill( CString directory, bool ignoreExtension )
{
	BW_GUARD;

	CWaitCursor waitCursor;

	SetRedraw(FALSE);

	DeleteAllItems();

	if (directory == "")
		return;

	if (directoryMask_ != "")
	{
		CString directoryLowercase = directory;
		directoryLowercase.MakeLower();
		if (directory.Find(directoryMask_) == -1)
			return;
	}

	// Xiaoming Shi : add an anti mask flag here, so anything
	// we doesn't like will also be filtered out{
	if( ! directoryAntiMask_.IsEmpty() )
	{
		CString directoryLowercase = directory;
		directoryLowercase.MakeLower();
		if( directory.Find( directoryAntiMask_ ) != -1 )
			return;
	}
	// Xiaoming Shi}
	int i = 0;
	bool imagesExist = false;
	for (BW::vector<CString>::iterator it = fileMask_.begin();
		it != fileMask_.end();
		it++)
	{
		CString searchString = directory + *it;

		CFileFind finder;
		if ( !finder.FindFile(searchString) )
			continue;

		BOOL moreFiles = TRUE;
		while ( moreFiles )
		{
			moreFiles = finder.FindNextFile();

			if ( !finder.IsDirectory() )
			{
				const CString filename = finder.GetFileName();
				CString fullFilename = directory + filename;

				// this is a hack for dds and bmp/tga
				// since we want to hide the dds when a bmp or a tga with the same name is available
				// currently we haven't a general override mechanism and it is not economical to add one
				BW::string nfullFilename;
				bw_wtoutf8( fullFilename, nfullFilename );
				if( BWResource::getExtension( nfullFilename ).equals_lower( "dds" ) )
				{
					if( BWResource::fileExists(
						BWResource::changeExtension( nfullFilename, ".tga" ) ) ||
						BWResource::fileExists(
						BWResource::changeExtension( nfullFilename, ".bmp" ) ) )
					continue;
				}

				if( fullFilename.Right( static_cast<int>(wcslen( THUMB_NAIL_SUFFIX )) ) == THUMB_NAIL_SUFFIX )
					continue;

				int imageIndex = findImageIndex(fullFilename);
				if (imageIndex != -1)
					imagesExist = true;

				if (!callbackFn_ || (callbackFn_)( bw_wtoutf8( fullFilename.GetBuffer() )))
				{
					if( ignoreExtension )
						InsertItem( LVIF_TEXT | LVIF_IMAGE, i++,
							bw_utf8tow( BWResource::removeExtension( bw_wtoutf8( filename.GetString() ) ).to_string() ).c_str(), 0, 0, imageIndex, NULL);
					else
						InsertItem( LVIF_TEXT | LVIF_IMAGE, i++, filename, 0, 0, imageIndex, NULL);
				}
			}
		}
	}

	UpdateData(TRUE);

	// Remove whatever style is there currently
	ModifyStyle(LVS_ICON | LVS_LIST | LVS_REPORT | LVS_SMALLICON ,0);

	if (!imagesExist)
	{
		ModifyStyle(0, LVS_SMALLICON);	// LVS_LIST
	}
	else
	{
		ModifyStyle(0, LVS_ICON);
	}

	SetRedraw(TRUE);
	Invalidate();
}

void FileImageList::setFileImageRep(const CString& fileExtension, 
							const CString& imageExtension)
{
	BW_GUARD;

	fileImageRepMap_.insert(std::pair<CString, CString>(fileExtension, imageExtension));
}

void FileImageList::addFileMask(const CString& mask)
{
	BW_GUARD;

	// make sure not already there
	if (std::find(fileMask_.begin(), fileMask_.end(), mask) != fileMask_.end())
		return;

	fileMask_.push_back(mask);
}

void FileImageList::setDirectoryMask(const CString& mask)
{
	BW_GUARD;

	directoryMask_ = mask;
	directoryMask_.MakeLower();
}

// Xiaoming Shi : add an anti mask flag here, so anything
// we doesn't like will also be filtered out{
void FileImageList::setDirectoryAntiMask(const CString& antiMask)
{
	BW_GUARD;

	directoryAntiMask_ = antiMask;
	directoryAntiMask_.MakeLower();
}
// Xiaoming Shi}

BEGIN_MESSAGE_MAP(FileImageList, CListCtrl)
END_MESSAGE_MAP()


BW_END_NAMESPACE

// FileImageList message handlers

