<?php
require_once('XHTML-MP-functions.php');

function renderFileContents( $filename, $tail = 10 )
{
    $contents = file( $filename );
    $lines = count( $contents );

    if ($tail == false || $lines < $tail)
	{
        $out = nl2br( implode( $contents, '' ) );
    }
    else
	{
        $out = nl2br( implode(
			array_slice( $contents,
				$lines - $tail, $tail ), '' ) );
    }

    return xhtmlMpTag( 'div', $out, '', ' style="font-family:Courier;"' );
}

function timeDiffMs( $t0, $t1 )
{
	return ($t1 - $t0) * 1000;
}


function timeFloatToString( $time, $granularity = FALSE )
{

	$timeUnits = Array( 60, 60, 24, 7 );
	$timeUnitNames = Array( "sec", "min", "hr", "day" );
	$out = '';

	$granDisabled = FALSE;
	if ($granularity && in_array( $granularity, $timeUnitNames ))
	{
		$granDisabled = TRUE;
	}

	$neg = FALSE;
	if ($time < 0 )
	{
		$neg = TRUE;
		$time = -$time;
	}

	foreach( $timeUnits as $key => $value )
	{
		$timeDiv = (int)($time / (float)$value );
		$timeMod = (int)($time - $timeDiv * $value);
		$timeUnitName = $timeUnitNames[$key];

		$time = (float)$timeDiv;
		if ($granDisabled)
		{
			if ($timeUnitName == $granularity)
			{
				$granDisabled = FALSE;
			}
			else
			{
				continue;
			}
		}

		$timeUnitString = $timeMod. " ". $timeUnitName;
		if ($timeMod > 1)
		{
			$timeUnitString .= 's';
		}

		if ($out != '')
		{
			if ($timeMod)
			{
				$out = $timeUnitString. " ".  $out;
			}
		}
		else
		{
			$out = $timeUnitString;
		}

		if ($timeDiv == 0)
		{
			break;
		}

	}

	if (!$out && $granularity)
	{
		return "< ". $granularity;
	}

	if ($neg)
	{
		return "-". $out;
	}
	return $out;

}


?>
