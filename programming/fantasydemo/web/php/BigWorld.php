<?php

require_once( "BWError.php" );
require_once( "CustomErrors.php" );

define( "NO_CONN", "Unable to contact game server" );
define( "KNOWN_URL", "bwServiceUrl" );

// Singleton class to store the list of available service URLs, and maintain
// the URL of a service known to be online, that will be used throughout the
// session.
class Service
{
	private function __construct()
	{
		// *** EDIT THIS WITH YOUR ServiceApp ADDRESSES ***
		$this->urlList = array(
			"http://localhost:13000"
			// e.g.
			// "http://someMachine:13000",
			// "http://localhost:13000",
			// "http://someOtherMachine:13000"
			);
	}

	private static $instance_;
	public $urlList;

	public static function instance()
	{
		if (!self::$instance_)
		{
			self::$instance_ = new Service();
		}

		return self::$instance_;
	}

	// Retrieve any stored URL
	public static function getUrl()
	{
		return $_SESSION[ KNOWN_URL ];
	}

	// Store a new known URL
	public static function setUrl($new_url)
	{
		$_SESSION[ KNOWN_URL ] = $new_url;
	}

	// Clear the stored URL
	public static function unsetUrl()
	{
		unset( $_SESSION[ KNOWN_URL ] );
	}
}

class RemoteEntity
{

	private $path_psomeOtherMachinerefix = '';

	function __construct( $entity_path )
	{
		$this->path_prefix = $entity_path . '/';
	}

	function argumentsString( $arguments )
	{
		if ($arguments)
		{
			return '?' . http_build_query( $arguments[0] );
		}

		return '';
	}

	// Attempt to connect to the given ServiceApp URL. If successful, make the
	// call. Return the results of the call, or any error value encountered.
	function attemptCall( $name, $arguments, $service_url )
	{
		$path = $service_url . '/' .
			$this->path_prefix . $name . $this->argumentsString( $arguments );

		$conn = curl_init( $path );

		curl_setopt( $conn, CURLOPT_HEADER, 0 );

		ob_start();
		$isOk = curl_exec( $conn );

		$httpCode = curl_getinfo( $conn, CURLINFO_HTTP_CODE );

		$output = ob_get_contents();
		ob_end_clean();

		curl_close( $conn );

		// No connection
		if (!$isOk)
		{
			throw new NoConnectionError( "Unable to contact game server" );
		}
		
		// 403 denotes an error object
		if ($httpCode == 403)
		{
			$result = json_decode( $output, TRUE );
		
			raiseBWError( $result[ 'excType' ], $result[ 'args' ]);
		}
		else if ($httpCode == 404)
		{
			throw new NoSuchURLError( "No such URL: " . $path );
		}
		else if ($httpCode < 200 || $httpCode > 299)
		{
			throw new BWGenericError( "HTTP code: $httpCode\nPath: $path" );
		}
		
		$result = json_decode( $output, TRUE );

		// Success
		return $result;
	}

	function findConnectionAndCall(
			$name, $arguments, $service_url_list )
	{

		// Randomise the url order. This will help to spread user load among all
		// available ServiceApps.
		shuffle( $service_url_list );

		foreach ($service_url_list as $service_url)
		{
			try
			{
				$result = $this->attemptCall( $name, $arguments, $service_url );
			}
			catch( NoConnectionError $e )
			{
				// If there is no connection, try the next ServiceApp
				continue;
			}

			Service::setUrl( $service_url );
			return $result;
		}

		// All ServiceApps failed
		Service::unsetUrl();
		throw new NoConnectionError( NO_CONN );
	}

	public function __call( $name, $arguments ) {

		// Try the url from the original connection
		$url = Service::getUrl();
		if (isset( $url ))
		{
			try
			{
				return $this->attemptCall( $name, $arguments, $url );
			}
			catch( NoConnectionError $e )
			{
				// Fall through to findConnectionAndCall
			}
			catch( Exception $e )
			{
				throw $e;
			}
		}

		// The stored ServiceApp is down, or this is a new session, so there is
		// not one stored. Try one of the other ServiceApps in the service
		// instance
		return $this->findConnectionAndCall(
			$name, $arguments, Service::instance()->urlList );
	}
}

?>
