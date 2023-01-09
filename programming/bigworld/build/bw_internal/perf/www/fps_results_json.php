<?php

$columns = array_keys( $_GET[ "column" ] );

//$hostname = $_GET[ "hostname" ];
//$executable = $_GET[ "executable" ];
//$branch = $_GET[ "branch" ];
//$benchmarkName = $_GET[ "benchmarkname" ];

$scriptDir = 'F:\\sys_admin\\scripts\\build';
chdir( $scriptDir );

$cmd = 'export_framerate_results.py';
$cmd .= ' -d MYSQL';

foreach($_GET[ "column" ] as $column)
{
	$parts = array();
	
	/*
	foreach (explode( ",", $column ) as $pair)
	{
		list($key,$value) = explode( ':', $pair );
		$parts[$key] = $value;
	}
		
	$hostname = $parts[ "hostname" ];
	$executable = $parts[ "executable" ];
	$branch = $parts[ "branch" ];
	$benchmarkName = $parts[ "benchmark" ];
	$columnName = $parts[ "name" ];
	
	$cmdPart = ('"' . $hostname . ',' . $executable . ',' .$branch . ',' . $benchmarkName . ',' . $columnName . '"');
	$cmd .= " " . $cmdPart;
	*/
	$cmd .= ' "' . $column . '"';
}

//die($cmd);

header( 'Cache-Control: no-cache, must-revalidate' );
header( 'Expires: Mon, 26 Jul 1997 05:00:00 GMT' );
header( 'Content-type: application/json' );
passthru( $cmd );

?>
