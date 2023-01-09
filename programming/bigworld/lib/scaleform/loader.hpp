#ifndef SCALEFORM_LOADER_HPP
#define SCALEFORM_LOADER_HPP
#if SCALEFORM_SUPPORT

#include "config.hpp"
#include "pyscript/script.hpp"
#include "resmgr/binary_block.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	class FileOpener : public GFx::FileOpener
	{
	public:
		virtual GFile* OpenFile(const char *pfilename, SInt flags = FileConstants::Open_Read | FileConstants::Open_Buffered,
			SInt mode = FileConstants::Mode_ReadWrite);
	};


	class BWToGFxImageLoader : public GFx::ImageCreator
	{
	public:
		BWToGFxImageLoader() : GFx::ImageCreator(NULL) {}
		virtual Render::Image *LoadProtocolImage(const GFx::ImageCreateInfo& info, const GString& url);
	protected:
		bool ParseStickerUrl( const char *szUrl, uint &width, uint &height, BW::string &strFilePath,
			uint &tileWidth, uint &tileHeight, uint &tileX, uint &tileY );
	};


	// We're deriving from GMemoryFile so that we can store the BinaryPtr on the file instance
	// rather than on the FileOpener singleton. This will avoid any problems that may arise if
	// scaleform decides it wants to read from two files at the same time (when stored on the singleton
	// and if you load a second file, the BinaryPtr will be replaced - potentially invalidating the 
	// original file's memory buffer).
	class BWGMemoryFile : public MemoryFile
	{
	public:
		BWGMemoryFile (const char *pFileName, BinaryPtr ptr)
			:	MemoryFile( pFileName, (const UByte*)ptr->cdata(), ptr->len() ),
				swfFile_(ptr)
		{
			BW_GUARD;
			//INFO_MSG("BWGMemoryFile::BWGMemoryFile - %s (BinaryPtr: 0x%p, len: %d, compressed: %d)\n", pFileName, ptr.get(), ptr->len(), ptr->isCompressed() );
		}

		virtual ~BWGMemoryFile()
		{
			BW_GUARD;
			//INFO_MSG("BWGMemoryFile::~BWGMemoryFile\n");
		}

	private:
		BinaryPtr swfFile_;
	};


	class Loader : public GFx::Loader
	{
	public:
		Ptr<GFx::FileOpener> pFileOpener_;

		Loader();
		~Loader();
	};


}	//namespace ScaleformBW

BW_END_NAMESPACE

#endif  //#if SCALEFORM_SUPPORT
#endif	//SCALEFORM_LOADER_HPP