#ifndef CUSTOM_HPP
#define CUSTOM_HPP

#include "network/basictypes.hpp"
#include "resmgr/datasection.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

// this file contains hook functions you can use to customise the
// database system

// if an entity doesn't exist when logging on, an implementation of
// IDatabase should call this function and add the result into the
// database; if this function returns NULL, then the IDatabase
// implementation should fail gracefully
// (function in IDatabase concerned is readOrCreateEntity)
DataSectionPtr createNewEntity( EntityTypeID, const BW::string & name );

BW_END_NAMESPACE

#endif // CUSTOM_HPP
