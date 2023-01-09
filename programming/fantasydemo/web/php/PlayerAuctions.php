<?php

/*
 * SearchAuctions.php
 * Allows player to search through auctions.
 */

// ----------------------------------------------------------------------------
// Section: Includes/Requires
// ----------------------------------------------------------------------------
require_once( 'XHTML-MP-functions.php' );
require_once( 'AuthenticatedPage.php' );
require_once( 'Items.php' );
require_once( 'Util.php' );


// ----------------------------------------------------------------------------
// Section: SearchAuctionsPage
// ----------------------------------------------------------------------------

class PlayerAuctionsPage extends AuthenticatedXHTMLMPPage
{
	var $playerAuctions;
	var $officialTime;
	var $player;
	var $auctionHouse;
	var $playerDBID;

	function PlayerAuctionsPage()
	{
		AuthenticatedXHTMLMpPage::AuthenticatedXHTMLMPPage( 'My Auctions' );
	}

	function initialise()
	{
		// set up class variables accessed later on
		$this->player = $this->auth->entity();
		$this->auctionHouse = new RemoteEntity( "global_entities/AuctionHouse" );

		try
		{
			$res = $this->player->webGetPlayerInfo();
			$this->playerDBID = $res["databaseID"];

			$res = $this->player->webGetGoldAndInventory();
			$this->availableGold = $res["goldPieces"];

			$res = $this->auctionHouse->webGetTime();
			$this->officialTime = $res["time"];
		}
		catch( Exception $e )
		{
			$this->addExceptionMsgAndDisconnect( $e );
			return;
		}

		$this->doGetPlayerAuctions();
	}


	function doGetPlayerAuctions()
	{
		$itemMgr =& ItemTypeManager::instance();

		try
		{
			$res = $this->auctionHouse->webCreateSellerCriteria( array(
				"dbID" => (int)$this->playerDBID ) );
		}
		catch( Exception $e )
		{
			$this->addExceptionMsgAndDisconnect( $e );
			return;
		}

		$sellerCriteria = $res[ "criteria" ];

		try
		{
			$res = $this->auctionHouse->webSearchAuctions( array(
				"criteria" => $sellerCriteria ) );
		}
		catch( Exception $e )
		{
			$this->addExceptionMsg( $e );
			return;
		}

		$res = $this->auctionHouse->webGetAuctionInfo( array(
			"auctions" => $res[ "searchedAuctions" ] ) );
		$this->playerAuctions = $res[ "auctionInfo" ];
	} //  function doSearchAction()



	function renderBody()
	{
		$this->renderErrorMsgs();
		$this->renderStatusMsgs();

		$this->printPlayerAuctions();

	}

	function printPlayerAuctions()
	{
		echo xhtmlMpHeading( 'My Auctions', 2 );
		if (!count( $this->playerAuctions ))
		{
			echo xhtmlMpPara(
				"You have created no auctions that are currently pending. ".
				"Go to ". xhtmlMpLink( "Inventory.php", "Inventory" ).
				" to create an auction from one of your inventory items." );
			return;
		}

		$rows = xhtmlMpTableRow(
			xhtmlMpTableHeader( "Item" ).
			xhtmlMpTableHeader( "Expiry" ).
			xhtmlMpTableHeader( "Current bid" ).
			xhtmlMpTableHeader( "Buyout price" )
		);

		$itemMgr =& ItemTypeManager::instance();

		$first = "-first";
		$count = 0;
		foreach ($this->playerAuctions as $auctionInfo )
		{
			$auctionId = $auctionInfo['auctionID'];
			$itemType = $auctionInfo['itemType'];
			$itemTypeName = $itemMgr->itemTypes[$itemType]->name;
			$itemTypeImgUrl = $itemMgr->itemTypes[$itemType]->iconImgUrl;


			$cells =
				xhtmlMpTableCell(
					xhtmlMpImg( $itemTypeImgUrl, $itemTypeName ).
					$itemTypeName
				).
				xhtmlMpTableCell(
					timeFloatToString( $auctionInfo['expiry'] -
						$this->officialTime )
				).
				xhtmlMpTableCell( $auctionInfo['currentBid']  ).
				xhtmlMpTableCell( $auctionInfo['buyoutPrice'] == FALSE ?
					"-": $auctionInfo['buyoutPrice'])
			;


			$rows .= xhtmlMpTableRow(
				$cells,
				"alt". (($count++ % 2) + 1). $first
			);
			$first = "";
		}

		echo xhtmlMpTable(
			$rows,
			'0', '', 'tradeTable'
		);

	}

}

// ----------------------------------------------------------------------------
// Section: Main body
// ----------------------------------------------------------------------------

$page = new PlayerAuctionsPage();
$page->render();
?>
