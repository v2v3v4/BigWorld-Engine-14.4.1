// ModelEditorDoc.h : interface of the CModelEditorDoc class
//


#pragma once

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class CModelEditorDoc : public CDocument
{
protected: // create from serialization only
	CModelEditorDoc();
	DECLARE_DYNCREATE(CModelEditorDoc)

// Attributes
public:

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CModelEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	static CModelEditorDoc& instance() { return *s_instance_; }

protected:
	static CModelEditorDoc * s_instance_;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

BW_END_NAMESPACE

