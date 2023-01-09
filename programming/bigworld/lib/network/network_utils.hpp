#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

BW_BEGIN_NAMESPACE

bool isErrnoIgnorable();
const char * lastNetworkError();
const char * networkErrorMessage( int error );

int getMaxBufferSize( bool isReadBuffer );

BW_END_NAMESPACE

#endif // NETWORK_UTILS_HPP
