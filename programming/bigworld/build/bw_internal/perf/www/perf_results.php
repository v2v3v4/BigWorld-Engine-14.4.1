<?php
header( 'Cache-Control: no-cache, must-revalidate' );
header( 'Expires: Mon, 26 Jul 1997 05:00:00 GMT' );
header( 'Content-type: application/json' );

$hostname = $_GET[ "hostname" ];
$executable = $_GET[ "executable" ];
$branch = $_GET[ "branch" ];
$benchmarkName = $_GET[ "benchmarkname" ];

$scriptDir = '../';
chdir( $scriptDir );

$cmd = 'performance_test.py';
$cmd .= ' -d SQLITE';
$cmd .= ' -e "' . $hostname . ',' . $branch . ',' .$benchmarkName . '" FPS ' . $executable;

//echo getcwd()."\n";
//echo $cmd."\n";

passthru( $cmd );

?>
