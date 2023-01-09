CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( libpython_tools )

INCLUDE( BWStandardProject )
INCLUDE( BWStandardLibrary )

BW_REMOVE_COMPILE_FLAGS( /WX )
ADD_DEFINITIONS( -DHAVE_LIBJPEG )

INCLUDE_DIRECTORIES( _imaging/libImaging )
INCLUDE_DIRECTORIES( jpeg-6b )

SET( _IMAGING_SRCS
	_imaging/_imaging.c
	_imaging/decode.c
	_imaging/encode.c
	_imaging/display.c
	_imaging/path.c
	_imaging/outline.c
	_imaging/map.c
	_imaging/libImaging/Access.c
	_imaging/libImaging/Antialias.c
	_imaging/libImaging/Bands.c
	_imaging/libImaging/BitDecode.c
	_imaging/libImaging/Blend.c
	_imaging/libImaging/Chops.c
	_imaging/libImaging/Convert.c
	_imaging/libImaging/ConvertYCbCr.c
	_imaging/libImaging/Copy.c
	_imaging/libImaging/Crc32.c
	_imaging/libImaging/Crop.c
	_imaging/libImaging/Dib.c
	_imaging/libImaging/Draw.c
	_imaging/libImaging/Effects.c
	_imaging/libImaging/EpsEncode.c
	_imaging/libImaging/File.c
	_imaging/libImaging/Fill.c
	_imaging/libImaging/Filter.c
	_imaging/libImaging/FliDecode.c
	_imaging/libImaging/Geometry.c
	_imaging/libImaging/GetBBox.c
	_imaging/libImaging/GifDecode.c
	_imaging/libImaging/GifEncode.c
	_imaging/libImaging/HexDecode.c
	_imaging/libImaging/Histo.c
	_imaging/libImaging/JpegDecode.c
	_imaging/libImaging/JpegEncode.c
	_imaging/libImaging/LzwDecode.c
	_imaging/libImaging/Matrix.c
	_imaging/libImaging/ModeFilter.c
	_imaging/libImaging/MspDecode.c
	_imaging/libImaging/Negative.c
	_imaging/libImaging/Offset.c
	_imaging/libImaging/Pack.c
	_imaging/libImaging/PackDecode.c
	_imaging/libImaging/Palette.c
	_imaging/libImaging/Paste.c
	_imaging/libImaging/PcdDecode.c
	_imaging/libImaging/PcxDecode.c
	_imaging/libImaging/PcxEncode.c
	_imaging/libImaging/Point.c
	_imaging/libImaging/Quant.c
	_imaging/libImaging/QuantHash.c
	_imaging/libImaging/QuantHeap.c
	_imaging/libImaging/RankFilter.c
	_imaging/libImaging/RawDecode.c
	_imaging/libImaging/RawEncode.c
	_imaging/libImaging/Storage.c
	_imaging/libImaging/SunRleDecode.c
	_imaging/libImaging/TgaRleDecode.c
	_imaging/libImaging/Unpack.c
	_imaging/libImaging/UnpackYCC.c
	_imaging/libImaging/UnsharpMask.c
	_imaging/libImaging/XbmDecode.c
	_imaging/libImaging/XbmEncode.c
	_imaging/libImaging/ZipDecode.c
	_imaging/libImaging/ZipEncode.c
)
SOURCE_GROUP( "_imaging" FILES ${_IMAGING_SRCS} )

SET( JPEG_SRCS
	jpeg-6b/jcapimin.c
	jpeg-6b/jcapistd.c
	jpeg-6b/jccoefct.c
	jpeg-6b/jccolor.c
	jpeg-6b/jcdctmgr.c
	jpeg-6b/jchuff.c
	jpeg-6b/jcinit.c
	jpeg-6b/jcmainct.c
	jpeg-6b/jcmarker.c
	jpeg-6b/jcmaster.c
	jpeg-6b/jcomapi.c
	jpeg-6b/jcparam.c
	jpeg-6b/jcphuff.c
	jpeg-6b/jcprepct.c
	jpeg-6b/jcsample.c
	jpeg-6b/jctrans.c
	jpeg-6b/jdapimin.c
	jpeg-6b/jdapistd.c
	jpeg-6b/jdatadst.c
	jpeg-6b/jdatasrc.c
	jpeg-6b/jdcoefct.c
	jpeg-6b/jdcolor.c
	jpeg-6b/jddctmgr.c
	jpeg-6b/jdhuff.c
	jpeg-6b/jdinput.c
	jpeg-6b/jdmainct.c
	jpeg-6b/jdmarker.c
	jpeg-6b/jdmaster.c
	jpeg-6b/jdmerge.c
	jpeg-6b/jdphuff.c
	jpeg-6b/jdpostct.c
	jpeg-6b/jdsample.c
	jpeg-6b/jdtrans.c
	jpeg-6b/jerror.c
	jpeg-6b/jfdctflt.c
	jpeg-6b/jfdctfst.c
	jpeg-6b/jfdctint.c
	jpeg-6b/jidctflt.c
	jpeg-6b/jidctfst.c
	jpeg-6b/jidctint.c
	jpeg-6b/jidctred.c
	jpeg-6b/jmemmgr.c
	jpeg-6b/jmemnobs.c
	jpeg-6b/jquant1.c
	jpeg-6b/jquant2.c
	jpeg-6b/jutils.c
)
SOURCE_GROUP( "jpeg" FILES ${JPEG_SRCS} )

ADD_LIBRARY( libpython_tools 
	${_IMAGING_SRCS}
	${JPEG_SRCS}
)

BW_PROJECT_CATEGORY( libpython_tools "ThirdParty" )
