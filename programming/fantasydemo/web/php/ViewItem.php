<?php
require_once( 'AuthenticatedPage.php' );
require_once( 'Constants.php' );

class ViewItemPage extends AuthenticatedXHTMLMPPage
{
	var $itemId;
	var $itemTypeInfo;

	function ViewItemPage()
	{
        AuthenticatedXHTMLMPPage::AuthenticatedXHTMLMPPage( "View Item" );
		$this->itemId = -1;
		$this->itemTypeInfo = NULL;
	}


	function initialise()
	{
		$itemId =& $_REQUEST['item'];
		if (strlen( $itemId ) == 0 		||
				!is_numeric( $itemId ) 	||
				(int)$itemId < 0 )
		{
			$this->itemId = -1;
		}
		else {
			$this->itemId = (int)$itemId;
		}

		// see if there's a Referrer to add a Back link for
		if (isset( $_REQUEST['referrer'] ))
		{
			$this->menubar->addItem( "Back", $_REQUEST['referrer'] );
		}

		if ($this->itemId != -1)
		{
			if(!array_key_exists( $this->itemId, $GLOBALS['BW_ITEM_TYPES'] ))
			{
				$this->setRedirect( 'ViewItem?referrer='. $_REQUEST['referrer'] );
				return;
			}

			$this->itemTypeInfo =& $GLOBALS['BW_ITEM_TYPES'][$this->itemId];
			$this->title = $this->itemTypeInfo['name'];

		}


	}


	function renderBody()
	{
	    echo( xhtmlMpHeading( $this->title ) );

		if ($this->itemId != -1)
		{
			echo(
				xhtmlMpDiv(
					xhtmlMpImg(
						$this->itemTypeInfo['img_href'],
						$this->itemTypeInfo['name']. " thumbnail",
						150
					),
					"itemThumbnail"
				)
			);
			echo(
				xhtmlMpPara(
					$this->itemTypeInfo['description'],
					"itemDescription"
				)
			);
		}
		else {
			echo(
				xhtmlMpPara(
					"The item you have chosen is not valid.",
					"error" )
			);
		}
	}
}

$page = new ViewItemPage();
$page->render();

?>
