#ifndef LOGINAPP_PUBLIC_KEY_HPP
#define LOGINAPP_PUBLIC_KEY_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

// g_loginAppPublicKey is an in code version of bigworld/res/loginapp.pubkey
// which is necessary for the PS3 / XBOX builds which don't use ResMgr.
#if defined( PLAYSTATION3 ) || defined( _XBOX360 ) || defined( __APPLE__ ) || defined( __ANDROID__ )
char g_loginAppPublicKey[] = "-----BEGIN PUBLIC KEY-----\n"\
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA7/MNyWDdFpXhpFTO9LHz\n"\
"CUQPYv2YP5rqJjUoxAFa3uKiPKbRvVFjUQ9lGHyjCmtixBbBqCTvDWu6Zh9Imu3x\n"\
"KgCJh6NPSkddH3l+C+51FNtu3dGntbSLWuwi6Au1ErNpySpdx+Le7YEcFviY/ClZ\n"\
"ayvVdA0tcb5NVJ4Axu13NvsuOUMqHxzCZRXCe6nyp6phFP2dQQZj8QZp0VsMFvhh\n"\
"MsZ4srdFLG0sd8qliYzSqIyEQkwO8TQleHzfYYZ90wPTCOvMnMe5+zCH0iPJMisP\n"\
"YB60u6lK9cvDEeuhPH95TPpzLNUFgmQIu9FU8PkcKA53bj0LWZR7v86Oco6vFg6V\n"\
"sQIDAQAB\n"\
"-----END PUBLIC KEY-----\n";
#endif

BW_END_NAMESPACE

#endif // LOGINAPP_PUBLIC_KEY_HPP
