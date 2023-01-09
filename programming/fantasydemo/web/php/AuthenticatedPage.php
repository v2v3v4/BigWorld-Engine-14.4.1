<?php

require_once( 'Session.php' );
require_once( 'Page.php' );
require_once( 'Authenticator.php' );
require_once( 'MenuBar.php' );
require_once( 'Constants.php' );

class AuthenticatedXHTMLMPPage extends XHTMLMPPage
{
	var $menubar;
	var $auth;

	var $errorMsgs;
	var $statusMsgs;

	function AuthenticatedXHTMLMPPage( $title,
		$authenticator=AUTHENTICATOR_CLASS )
	{
		if (is_null($authenticator) || $authenticator == FALSE)
		{
			$this->auth = new Authenticator();
		}
		else
		{
			$this->auth = new $authenticator();
		}
		XHTMLMPPage::XHTMLMPPage( $title );
		$this->menubar = new MenuBar();
		$this->menubar->addItem( 'Home', 'News.php' );
		$this->menubar->addItem( 'Character', 'Character.php' );
		$this->menubar->addItem( 'Inventory', "Inventory.php" );
		//$this->menubar->addItem( 'BuyItems', 'BuyItems.php' );
		//$this->menubar->addItem( 'MyShop', 'SellItems.php' );
		$this->menubar->addItem( 'AuctionHouse', 'SearchAuctions.php' );
		$this->menubar->addItem( 'MyAuctions', 'PlayerAuctions.php' );
		$this->menubar->addItem( 'Logout', 'Login.php?logout=1' );

		$this->statusMsgs = Array();
		$this->errorMsgs = Array();

	}


	function backToLogin( $args=Array() )
	{
		$this->auth->invalidateToken();

		$encoded = array();
		if ($args != false)
		{
			foreach ($args as $key => $value)
			{
				$encoded[] = urlencode( $key ). '='. urlencode( $value );
			}
		}

		if (count($encoded) > 0)
		{
			header( 'Location: '. LOGIN_PAGE_URL. '?'.
				implode( '&', $encoded ) );
			session_write_close();
			exit;
		}
		else
		{
			header( 'Location: '. LOGIN_PAGE_URL );
			session_write_close();
			exit;
		}
	}


	function renderErrorMsgs()
	{
		if (count( $this->errorMsgs ))
		{
			echo(
				xhtmlMpDiv(
					implode( xhtmlMpSingleTag( 'br' ), $this->errorMsgs ),
					'error'
				)
			);
		}
	}


	function renderStatusMsgs()
	{
		if (count( $this->statusMsgs ))
		{
			echo(
				xhtmlMpDiv(
					implode( xhtmlMpSingleTag( 'br' ), $this->statusMsgs ),
					'status'
				)
			);
		}
	}

	function render()
	{
		deferErrorPrinting( TRUE );
		$err = Array();
		if ($this->auth->doesAuthTokenExist())
		{
			try
			{
				$this->auth->authenticateSessionToken();
			}
			catch( Exception $e )
			{
				$this->addExceptionMsg( $e );
			}
		}
		else
		{
			# no auth token
			# save return URL
			$_SESSION['err_msgs'] = Array( "You are not logged in." );
			$this->backToLogin();
		}

		if (count( $err ) > 0)
		{
			# boot back to login if authentication failed
			$_SESSION['err_msgs'] = $err;
			$this->backToLogin();
		}
		else
		{
			session_unregister( 'err_msgs' );
		}

		$this->auth->updateLastActivity();

		$errorMsgs =& $this->auth->getVariable( "errorMsgs" );
		if (isset( $errorMsgs ))
		{
			$this->errorMsgs += $errorMsgs;
			$this->auth->setVariable( "errorMsgs", NULL );
		}

		$statusMsgs =& $this->auth->getVariable( "statusMsgs" );
		if (isset( $statusMsgs ))
		{
			$this->statusMsgs += $statusMsgs;
			$this->auth->setVariable( "statusMsgs", NULL );
		}


		$this->doInitialise();

		if ($this->redirect)
		{
			header( 'Location: '. $this->redirect );
			exit;
		}

		header( 'Content-Type: '. $this->contentType );


		echo( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
		echo( $this->docType."\n" );


		echo( "<html>\n" );

		deferErrorPrinting( FALSE );

		debug( "PHP_SELF: ". $_SERVER['PHP_SELF'] );

		echo( "<head>\n" );
		$this->doRenderHead();
		echo( "</head>\n" );

		echo( "<body>\n" );
		echo( xhtmlMpHeading(
			'&middot;'.
				strtoupper( $this->title ).
				'&middot;',
			1,
			'heading'
		) );
		$this->doRenderBody();
		echo( $this->menubar->contents() );
		echo( "</body>\n" );
		debug( 'Init time: ' . $this->initTime. 'ms'.
			"\nRenderBody time: ". $this->renderBodyTime. 'ms' );

		echo ( "</html>\n" );


	}

	function setRedirect( $url )
	{
		if (count( $this->errorMsgs ))
		{
			$this->auth->setVariable( "errorMsgs", $this->errorMsgs );
		}
		if (count( $this->statusMsgs ))
		{
			$this->auth->setVariable( "statusMsgs", $this->statusMsgs );
		}
		session_write_close();
		XHTMLMPPage::setRedirect( $url );
	}

}
?>
