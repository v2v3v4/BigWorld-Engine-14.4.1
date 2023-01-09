// ModelEditorDoc.cpp : implementation of the CModelEditorDoc class
//

#include "pch.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "model_editor_doc.h"
#include <afx.h>
#include <afxwin.h>

DECLARE_DEBUG_COMPONENT2( "ModelEditor", 0 )

BW_BEGIN_NAMESPACE

CModelEditorDoc * CModelEditorDoc::s_instance_ = NULL;

// CModelEditorDoc

IMPLEMENT_DYNCREATE(CModelEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CModelEditorDoc, CDocument)
END_MESSAGE_MAP()


// CModelEditorDoc construction/destruction

CModelEditorDoc::CModelEditorDoc()
{
	// TODO: add one-time construction code here

}

CModelEditorDoc::~CModelEditorDoc()
{
	s_instance_ = NULL;
}

BOOL CModelEditorDoc::OnNewDocument()
{
	BW_GUARD;

	ASSERT(!s_instance_);
	s_instance_ = this;

	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialisation code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CModelEditorDoc serialization

void CModelEditorDoc::Serialize(CArchive& ar)
{
	BW_GUARD;

	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CModelEditorDoc diagnostics

#ifdef _DEBUG
void CModelEditorDoc::AssertValid() const
{
	BW_GUARD;

	CDocument::AssertValid();
}

void CModelEditorDoc::Dump(CDumpContext& dc) const
{
	BW_GUARD;

	CDocument::Dump(dc);
}
#endif //_DEBUG

BW_END_NAMESPACE

// CModelEditorDoc commands
