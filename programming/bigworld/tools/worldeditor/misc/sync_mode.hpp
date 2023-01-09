#ifndef SYNC_MODE_HPP
#define SYNC_MODE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This object gives exclusive access to chunks while it is in scope.
 */
class SyncMode
{
public:
    explicit SyncMode();
    ~SyncMode();

    operator bool() const;

private:
    SyncMode(SyncMode const &);
    SyncMode &operator=(SyncMode const &);

private:
    bool            readOnly_;
};

BW_END_NAMESPACE

#endif // ELEVATION_MODIFY_HPP
