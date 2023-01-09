#ifndef RESMGR_FORWARD_DECLARATIONS_HPP
#define RESMGR_FORWARD_DECLARATIONS_HPP
#pragma once

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class BinaryBlock;
typedef SmartPointer<BinaryBlock> BinaryBlockPtr;

class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

BW_END_NAMESPACE

#endif // RESMGR_FORWARD_DECLARATIONS_HPP
