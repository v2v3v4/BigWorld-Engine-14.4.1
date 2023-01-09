#include "pch.hpp"

#include "snap_provider.hpp"

BW_BEGIN_NAMESPACE

SnapProvider* SnapProvider::s_instance_ = NULL;
bool SnapProvider::s_internal_ = true;

BW_END_NAMESPACE

// snap_provider.cpp