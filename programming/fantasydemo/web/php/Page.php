<?php
require_once ( 'Debug.php' );
require_once ( 'Constants.php' );
require_once ( 'XHTML-MP-functions.php' );
require_once( 'Util.php');
error_reporting( E_ALL );


class XHTMLMPPage
{
    var $contentType;
    var $docType;
    var $title;
    var $styleHref;

    var $redirect;

    var $initTime;
    var $renderBodyTime;

    function XHTMLMPPage( $title )
	{
        $this->title = $title;
        $this->docType = DOCTYPE_HTML;
        $this->contentType = CONTENTTYPE_HTML;

        $this->styleHrefs = Array( DEFAULT_STYLE );

        $this->redirect = false;

        $this->initTime = 0;
        $this->renderBodyTime = 0;
    }

	function getDevice()
	{
		if (isset( $_SESSION['useragent-info'] ))
		{
			return $_SESSION['useragent-info']['brand_name'].
				' '.
				$_SESSION['useragent-info']['model_name'];
		}

		return FALSE;
	}

    function initialise()
	{
        // do nothing - override in subclasses to process
        // request variables
    }

    function setRedirect($url)
	{
        $this->redirect = $url;
    }

    function renderHead()
	{
        echo( xhtmlMpTag( 'title', $this->title ) );

        foreach ($this->styleHrefs as $styleHref)
		{
        	$attrString = '';
	        $attrString = xhtmlMpAddAttribute( $attrString, 'rel',
    	        'stylesheet' );
			$attrString = xhtmlMpAddAttribute( $attrString, 'href',
            	$styleHref );
        	echo( xhtmlMpSingleTag( 'link', '', $attrString ). "\n" );
		}

		echo( xhtmlMpSingleTag( 'meta', '', 'name="viewport" content="width=600"' ) ); 

    }

    function renderBody()
	{
    }

    function doInitialise()
	{
        $initStart = microtime();
        $this->initialise();
        $initFinish = microtime();
        $this->initTime = timeDiffMs( $initStart, $initFinish );
    }

    function doRenderBody()
	{
        $renderBodyStart = microtime();
        $this->renderBody();
        $renderBodyFinish = microtime();

        $this->renderBodyTime = timeDiffMs(
			$renderBodyStart, $renderBodyFinish );
    }

    function doRenderHead()
	{
        $this->renderHead();
    }

    function render()
	{
		deferErrorPrinting( TRUE );

		$this->doInitialise();
        if ($this->redirect)
		{
            header( 'Location: '. $this->redirect );
            exit;
        }
    //    header( 'Content-Type: '. $this->contentType );

		echo( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
        echo( $this->docType."\n" );

		echo( "<html>\n" );

		deferErrorPrinting( FALSE );

		debug( "PHP_SELF: ". $_SERVER['PHP_SELF'] );

        echo( "<head>\n" );

        $this->renderHead();
        echo( "</head>\n" );

        echo( "<body>\n" );

        $this->doRenderBody();

        echo( "</body>\n" );

        debug( 'Init time: ' . $this->initTime. 'ms'.
            "\nRenderBody time: ". $this->renderBodyTime. 'ms' );
        echo( "</html>\n" );
    }

	function getExceptionMsg( $exception )
	{
		if ($exception instanceof BWError)
		{
			return $exception->message();
		}
		else
		{
			return $exception->getMessage();
		}
	}

	function addExceptionMsg( $exception, $additionalInfo = "" )
	{
		$msg = $this->getExceptionMsg( $exception );
		if ($additionalInfo != "")
		{
			$msg = "$additionalInfo: $msg";
		}
		$this->addErrorMsg( $msg );
		$_SESSION[ 'err_msgs'][] = $msg;
	}

	function addExceptionMsgAndDisconnect( $exception, $additionalInfo = "" )
	{
		$this->addExceptionMsg( $exception, $additionalInfo );
		$this->setRedirect( 'Disconnect.php' );
	}

	function addErrorMsg( $errmsg )
	{
		$this->errorMsgs[] = $errmsg;
	}

	function addStatusMsg( $statusmsg )
	{
		$this->statusMsgs[] = $statusmsg;
	}
}
?>
