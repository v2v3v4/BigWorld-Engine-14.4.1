#include "pch.hpp"
#include "particle_editor.hpp"
#include "gui/propdlgs/psa_empty_properties.hpp"

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(PsaEmptyProperties, PsaProperties)

BEGIN_MESSAGE_MAP(PsaEmptyProperties, PsaProperties)
END_MESSAGE_MAP()

PsaEmptyProperties::PsaEmptyProperties()
: 
PsaProperties(PsaEmptyProperties::IDD)
{
}

PsaEmptyProperties::~PsaEmptyProperties()
{
}

/*virtual*/ void *PsaEmptyProperties::action() 
{ 
    return NULL; 
}

/*virtual*/ void PsaEmptyProperties::SetParameters(SetOperation /*task*/) 
{ 
}
BW_END_NAMESPACE

