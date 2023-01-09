// ParticleEditorDoc.cpp : implementation of the ParticleEditorDoc class
//

#include "pch.hpp"
#include "particle_editor_doc.hpp"
#include "particle_editor.hpp"


DECLARE_DEBUG_COMPONENT2( "ParticleEditor", 0 )

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(ParticleEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(ParticleEditorDoc, CDocument)
END_MESSAGE_MAP()

ParticleEditorDoc * ParticleEditorDoc::s_instance_ = NULL;

ParticleEditorDoc::ParticleEditorDoc()
: 
CDocument(),
m_actionSelect(0), 
m_check(false)
{
	BW_GUARD;

	ASSERT(!s_instance_);
	s_instance_ = this;
}

ParticleEditorDoc::~ParticleEditorDoc()
{
	s_instance_ = NULL;
}

BOOL ParticleEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}

void ParticleEditorDoc::SetActionProperty(int actionSelect) 
{ 
    m_actionSelect = actionSelect; 
}

/*static*/ ParticleEditorDoc &ParticleEditorDoc::instance() 
{ 
    return *s_instance_; 
}
BW_END_NAMESPACE

