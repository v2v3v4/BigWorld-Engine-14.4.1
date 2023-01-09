#pragma once


#include "worldeditor/resource.h"
#include "controls/embeddable_cdialog.hpp"
#include "controls/edit_numeric.hpp"
#include "afxwin.h"

#define MAX_CHUNK_SIZE		1000
#define MIN_EXPAND_COUNT	0

BW_BEGIN_NAMESPACE

class ExpandSpaceDlg;

/**
 * 
 * Interface for receiving  validation changes to the expand space control.
 * 
 */
class IValidationChangeHandler
{
public:
	virtual void validationChange( bool validationResult ) = 0;
};

/**
 *
 * Reusable expand space control
 *
 */
class ExpandSpaceControl : public controls::EmbeddableCDialog
{
	DECLARE_DYNAMIC( ExpandSpaceControl )
public:
	struct ExpandInfo
	{
		ExpandInfo() : 
			west_(0), 
			east_(0), 
			north_(0), 
			south_(0),
			includeTerrainInNewChunks_(true) {}
		int west_;
		int east_;
		int north_;
		int south_;
		bool includeTerrainInNewChunks_;
	};

public:
	enum { IDD = IDD_EXPAND_SPACE_CONTROL };

	ExpandSpaceControl( IValidationChangeHandler * validationChangeHandler );
	virtual ~ExpandSpaceControl() {}

	const ExpandInfo& expandInfo();

	afx_msg void OnBnClickedExpandSpaceBtnOk();
	afx_msg void OnBnClickedExpandSpaceBtnCancel();
	afx_msg void OnEnChangeEdit();
	afx_msg void OnBnClickedCheckEmptyChunk();

protected:
	virtual void DoDataExchange( CDataExchange * pDX );

	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
	void init();

	friend class ExpandSpaceDlg;

private:
	controls::EditNumeric editWest_;
	controls::EditNumeric editEast_;
	controls::EditNumeric editNorth_;
	controls::EditNumeric editSouth_;
	CButton includeTerrain_;

	int maxToExpandX_;
	int maxToExpandY_;
	ExpandInfo expandInfo_;		
	IValidationChangeHandler * validationChangeHandler_;
};

/**
 *
 * Dialog to host the actual expand space control 
 *
 */
class ExpandSpaceDlg
	: public CDialog
	, public IValidationChangeHandler
{
	DECLARE_DYNAMIC( ExpandSpaceDlg )
public:
	enum { IDD = IDD_EXPAND_SPACE };

	ExpandSpaceDlg( CWnd * pParent = NULL );
	virtual ~ExpandSpaceDlg() {}

	const ExpandSpaceControl::ExpandInfo & expandInfo();

	afx_msg void OnBnClickedExpandSpaceBtnOk();
	afx_msg void OnBnClickedExpandSpaceBtnCancel();
	afx_msg void OnEnChangeEdit();
	afx_msg void OnBnClickedCheckEmptyChunk();

	virtual void validationChange( bool validationResult );

protected:
	virtual void DoDataExchange( CDataExchange * pDX );

	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
	void init();

private:
	CButton btnExpand_;
	ExpandSpaceControl expandSpaceControl_;
};

BW_END_NAMESPACE

