<?php

$columns = array_keys( $_GET[ "column" ] );

$type = $_GET[ "type" ];

$scriptDir = '../';
chdir( $scriptDir );

if ($type == "fps_regression" || $type == "fps_absolute")
{
	$cmd = 'python export_framerate_results.py';
}
else if ($type == "loadtime")
{
	$cmd = 'python export_loadtime_results.py';
}
else if ($type == "peak_mem_usage")
{
	$cmd = 'python export_smoketest_results.py -c peakAllocatedBytes';
}
else if ($type == "total_mem_allocs")
{
	$cmd = 'python export_smoketest_results.py -c totalMemoryAllocations';
}
else
{
	die( "Invalid 'type' parameter. Must be either 'fps', 'loadtime', 'total_mem_allocs' or 'peak_mem_usage." );
}
$cmd .= ' -d MYSQL';

foreach($_GET[ "column" ] as $column)
{
	$parts = array();
	$cmd .= ' "' . $column . '"';
}

//die( getcwd() . "\n" . $cmd );
//die($cmd);
#print($cmd);
header( 'Cache-Control: no-cache, must-revalidate' );
header( 'Expires: Mon, 26 Jul 1997 05:00:00 GMT' );
header( 'Content-type: application/json' );
passthru( $cmd );

?>
