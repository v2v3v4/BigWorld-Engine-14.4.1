#include "pch.hpp"

#include "coord_mode_provider.hpp"

BW_BEGIN_NAMESPACE

CoordModeProvider* CoordModeProvider::s_instance_ = NULL;
bool CoordModeProvider::s_internal_ = true;

BW_END_NAMESPACE
// coord_mode_provider.cpp