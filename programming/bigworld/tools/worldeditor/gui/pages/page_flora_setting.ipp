// page_flora_setting.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif

BW_BEGIN_NAMESPACE

INLINE
FloraSettingSecondaryWnd& PageFloraSetting::secondaryWnd()
{ 
	return secondaryWnd_; 
}


INLINE
FloraSettingTree& PageFloraSetting::tree()
{
	return floraSettingTree_; 
}


INLINE
SmartListCtrl&	PageFloraSetting::assetList()
{
	return secondaryWnd_.assetList(); 
}


INLINE
SmartListCtrl&	PageFloraSetting::existingResourceList()
{
	return secondaryWnd_.existResourceList(); 
}


INLINE
ListXmlSectionProviderPtr PageFloraSetting::listXmlSectionProvider()
{
	return listXmlSectionProvider_; 
}


INLINE
ListFileProviderPtr PageFloraSetting::fileScanProvider()
{ 
	return fileScanProvider_; 
}


INLINE
FixedListFileProviderPtr PageFloraSetting::existingResourceProvider()
{ 
	return existingResourceProvider_; 
}


INLINE
bool PageFloraSetting::toggle3DView()
{
	return toggle3DView_;
}


INLINE
bool PageFloraSetting::isReady()
{
	return initialised_;
}

BW_END_NAMESPACE

