#ifndef TEXTURE_FEED_HPP
#define TEXTURE_FEED_HPP

#include "resource.h"       // main symbols
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class CTextureFeed : public CDialog
{
public:
	enum { IDD = IDD_TEXTURE_FEED };
	
	CTextureFeed( const BW::wstring& feedName );

	virtual BOOL OnInitDialog();

	BW::wstring feedName() { return feedName_; }


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
private:
	void checkComplete();

	BW::wstring feedName_;
	
	CEdit name_;
	
	CButton ok_;


public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEnChangeTextureFeedName();
	afx_msg void OnBnClickedRemoveTextureFeed();
};
BW_END_NAMESPACE

#endif TEXTURE_FEED_HPP
