#ifndef FILE_IMAGE_LIST_HPP
#define FILE_IMAGE_LIST_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class FileImageList : public CListCtrl
{
	DECLARE_DYNAMIC(FileImageList)

public:
	FileImageList();
	virtual ~FileImageList();

	void initialise();
	void fill( CString directory, bool ignoreExtension = false );

	// directory mask is a string that must be contained in the directory name
	void setDirectoryMask(const CString& mask);

	// Xiaoming Shi : add an anti mask flag here, so anything
	// we doesn't like will also be filtered out{
	void setDirectoryAntiMask(const CString& antiMask);
	// Xiaoming Shi}

	// file mask uses a DOS wild card match type
	void addFileMask(const CString& mask);
	void setFileImageRep(const CString& fileExtension, 
							const CString& imageExtension);

	typedef bool(*FunctionPtr)(const BW::string& );
	void setCallbackFn(FunctionPtr fn) { callbackFn_ = fn; }

protected:
	DECLARE_MESSAGE_MAP()

private:
	CImageList imageList_;

	typedef BW::map<CString, int> StringIntMap;
	StringIntMap imageFileMap_;

	CString directoryMask_;
	// Xiaoming Shi : add an anti mask flag here, so anything
	// we doesn't like will also be filtered out{
	CString directoryAntiMask_;
	// Xiaoming Shi}
	BW::vector<CString> fileMask_;

	FunctionPtr callbackFn_;

	typedef BW::map<CString, CString> StringStringMap;
	StringStringMap fileImageRepMap_;

	int addImage(const CString& fullFilename);
	int addImage(CBitmap * bmpImage);
	int findImageIndex(const CString& fullFilename);
};

BW_END_NAMESPACE

#endif // FILE_IMAGE_LIST_HPP
