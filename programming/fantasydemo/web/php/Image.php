<?php
/**
 * Image wrapper to do dynamic resizing.
 */

require_once( 'Constants.php' );
require_once( 'Util.php' );
require_once( 'Debug.php' );

function makeIntFromBinary( $binaryString, $offset = 0, $len = 4 )
{
	$out = 0;
	for ($i = $offset; $i < $offset + $len; ++$i)
	{
		$out |= ord($binaryString[$i]) << (($i - $offset) * 8);
	}
	return $out;
}

function loadBmp( $filename )
{
	debug( "Image.php:loadBmp( $filename )" );
	$start = microtime();

	//	this is going to be very slow for large bitmaps - for 64x64 it takes
	//	~0.1s
	$contents = file_get_contents( $filename );

	// despite there being a C function which traverses the contents and stops
	// at a null terminator, PHP's strlen function (depending on the version,
	// grr) relies on an internal count.
	$fileSize = strlen( $contents );

	if (!substr( $contents, 0, 2 ) == "BM")
	{
		return FALSE;
	}

	$width = makeIntFromBinary( $contents, 18 );
	$height = makeIntFromBinary( $contents, 22 );

	$img = imageCreateTrueColor( $width, $height );
	$bmpOffset = makeIntFromBinary( $contents, 10 );

	$x = 0;
	$y = $height - 1;

	while ($bmpOffset < $fileSize && $y >= 0 )
	{
		if ($bmpOffset + 2 >= $fileSize)
		{
			debug( "uhoh at $x, $y" );
		}
		$blue = makeIntFromBinary( $contents, $bmpOffset, 1 );
		$green = makeIntFromBinary( $contents, $bmpOffset + 1, 1 );
		$red = makeIntFromBinary( $contents, $bmpOffset + 2, 1 );

		$colour = imageColorAllocate( $img, $red, $green, $blue );
		imageSetPixel( $img, $x, $y, $colour );
		imageColorDeallocate($img, $colour);
		$bmpOffset += 3;
		if (++$x == $width)
		{
			$y--;
			$x = 0;
		}
	}

	$finish = microtime();

	debug( "loadBmp took ". timeDiffMs( $start, $finish ). "ms" );

	return $img;
}

function doImage( $filename )
{
	$periodPos = strrpos( basename( $filename ), '.' );

	if ($periodPos !== FALSE)
	{
		$filename_noext = substr( basename( $filename ), 0, $periodPos );
		$filename_ext = substr( basename( $filename ), $periodPos + 1);
	} else {
		$filename_noext = basename( $filename );
		$filename_ext = '';
	}

	if ($filename_ext == 'bmp')
	{
		# GD doesn't support standard Windows BMP files?!
		$start = microtime();
		$inImage = loadBmp( $filename );
		error_log( "Time for bmp loading: ".
			timeDiffMs( $start, microtime() ) );
	}
	else
	{
		$inImage = imageCreateFromString( file_get_contents( $filename ) );
	}

	if (!$inImage)
	{
		$inImage = imageCreateFromWBMP ( $filename );
		if (!$inImage)
		{
		   header( "Content-Type: image/png" );
		   header( 'Content-Disposition: filename=error.png' );
		   $im  = imageCreateTrueColor ( 300, 100 ); /* Create a blank image */
		   $bgc = imageColorAllocate( $im, 255, 255, 255 );
		   $tc  = imageColorAllocate( $im, 0, 0, 0 );
		   imageFilledRectangle( $im, 0, 0, 300, 100, $bgc );
		   // Output an errmsg
		   imageString( $im, 5, 20, 20, "Error loading", $tc );
		   imageString( $im, 1, 20, 40, basename( $filename ), $tc );

		   imagePNG( $im );

		   exit();
		}

	}

	$dstWidth = $width = imageSX( $inImage );
	$dstHeight = $height = imageSY( $inImage );

	if ( isset( $_REQUEST[ 'max_height' ] ) )
	{
		$maxHeight = (int)$_REQUEST[ 'max_height' ];
	}
	else
	{
		$maxHeight = FALSE;
	}

	if (isset( $_REQUEST[ 'max_width' ] ))
	{
		$maxWidth = (int)$_REQUEST[ 'max_width' ];
	}
	else
	{
		$maxWidth = FALSE;
	}

	$scalingFactor = 1.0;
	if ($maxHeight || $maxWidth)
	{

		if ($maxHeight && $height > $maxHeight)
		{
			$newScalingFactor = (float)$maxHeight / (float)$height;
			$scalingFactor = min( $scalingFactor, $newScalingFactor );
		}

		if ( $maxWidth && $width > $maxWidth )
		{
			$newScalingFactor = (float)$maxWidth /
				(float)($scalingFactor * $width);
			$scalingFactor = min( $scalingFactor, $newScalingFactor );

		}
		$dstHeight = (int)($scalingFactor * $height);
		$dstWidth = $scalingFactor * $width;

	}


	$outImage = imageCreateTrueColor( $dstWidth, $dstHeight );
	imageCopyResampled( $outImage, $inImage,
		0, 0, 	// dst x, y
		0, 0, 	// src x, y
		$dstWidth,
		$dstHeight,
		$width, // src width
		$height // src height
	);

	header( 'Content-Type: image/jpeg' );
	header( 'Content-Disposition: filename="'. $filename_noext. '.jpg' );
	imagePNG( $outImage );
}

deferErrorPrinting( TRUE );

if (!isset( $_REQUEST['path'] ))
{
	header( "HTTP/1.1 404 Not Found" );
	exit();
}
else if (substr( $_REQUEST['path'], 0, strlen( "res/" ) ) == "res/")
{
	$filename = RES_BASE_DIR. substr( $_REQUEST['path'], strlen( 'res/' ) );
	doImage( $filename );
}
else if( ereg( '^[^./]', $_REQUEST['path'] ))
{
	$filename = $_REQUEST['path'];
	doImage( $filename );
}
else
{
	header( "HTTP/1.1 400 Bad Request" );
	exit();
}

?>
