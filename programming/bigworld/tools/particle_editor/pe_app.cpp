#include "pch.hpp"
#include "pe_app.hpp"

BW_BEGIN_NAMESPACE

PeApp *PeApp::s_instance = NULL;

PeApp::PeApp()
{
    s_instance = this;
}

PeApp::~PeApp()
{
    s_instance = NULL;
}


/*static*/ PeApp &PeApp::instance()
{
    return *s_instance;
}

BW_END_NAMESPACE

