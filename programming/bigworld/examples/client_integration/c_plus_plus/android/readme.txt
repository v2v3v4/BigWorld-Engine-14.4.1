SETUP

1. Download and install Android SDK and NDK:

http://developer.android.com/sdk/index.html

http://developer.android.com/tools/sdk/ndk/index.html

2. [Option 1] Install Eclipse Juno and then install Android NDK through
'Help->Install New Software'

2. [Option 2] To build and run the application through command line, make sure
you've got ant version >= 1.8 installed

http://ant.apache.org/bindownload.cgi
http://ant.apache.org/manual/index.html

export ANT_HOME=/usr/local/ant
export PATH=${PATH}:${ANT_HOME}/bin

3. Add paths to NDK executables to your PATH environment variable (Linux):
 
export PATH=${PATH}:${HOME}/android-sdks/tools:${HOME}/android-sdks/platform-tools:${HOME}/android-sdks/android-ndk-r8e

4. Now, being in the project directory, you should be able to build the example project just 
by running 'make' (Linux).
Run 'make generated' to generate source files with entity classes only.
Run 'ndk-build' to compile the native library without generating sources.

Note this step only builds the native .so file for your application, it doesn't build
the application itself.  

5. In order to build the example application on an android device from the
Command Line, run the following commands:

'android update project -p . -t android-15 -n "AndroidClient"'
'ant debug'

That will output a debug-signed APK as bin/AndroidClient-debug.apk, targeting
Android API 15 (Android 4.0.3)

For more details, see this manual:

http://developer.android.com/tools/building/building-cmdline.html    

Otherwise you'll have to use Eclipse or other IDE (Eclipse is preferable as the whole NDK
is Eclipse-centric).

INTEGRATION

1. BigWorld libraries required:
connection
connection_model
cstdmf
math
network
zip

2. External openssl
https://github.com/fries/android-external-openssl/downloads

is installed to src/lib/openssl_android

modifications made to crypto/Android.mk and ssl/Android.mk according to this fix:

http://www4.atword.jp/cathy39/2012/07/29/how-to-build-libcurl-for-android-with-ssl-as-a-static-library-1/


COMPILATION ISSUES

1. On Linux, the compiler may spit out warnings
 
/usr/local/lib/libz.so.1: no version information available
 
Ignore them.
