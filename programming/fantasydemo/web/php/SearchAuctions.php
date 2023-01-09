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
require_once( 'BigWorld.php' );

// ----------------------------------------------------------------------------
// Section: SearchAuctionsPage
// ----------------------------------------------------------------------------

class SearchAuctionsPage extends AuthenticatedXHTMLMPPage
{
	var $searchResults;
	var $playerBids;
	var $officialTime;
	var $player;
	var $auctionHouse;
	var $playerDBID;
	var $availableGold;

	/**
	 * Create a search auctions page.
	 */
	function SearchAuctionsPage()
	{
		AuthenticatedXHTMLMpPage::AuthenticatedXHTMLMPPage( 'Search Auctions' );
	}

	/**
	 * Perform a bid from a HTTP form post.
	 */
	function doBidAction( $auctionID )
	{
		$bidAmount = (int)($_REQUEST['bid_amount']);
		if ($bidAmount <= 0)
		{
			$this->addErrorMsg( "Bid amount is invalid." );
			return;
		}

		try
		{
			$res = $this->player->webBidOnAuction( array(
				   "auctionID" =>	$auctionID,
				   "isBuyingOut" => FALSE,
				   "bidAmount" => $bidAmount ) );
		}
		catch( Exception $e )
		{
			// error - should log out
			$this->addExceptionMsgAndDisconnect( $e );
			return;
		}

		$this->addStatusMsg( $res["status"] );

	} // function doBidAction()


	/**
	 * Perform a buyout from a HTTP form post.
	 */
	function doBuyoutAction( $auctionID )
	{
		try
		{
			$res = $this->player->webBidOnAuction( array(
					"auctionID" => $auctionID,
					"isBuyingOut" => TRUE,
					"bidAmount" => 0 ) );
		}
		catch( Exception $e )
		{
			$this->addExceptionMsg( $e );
			return;
		}
		$this->addStatusMsg( $res["status"] );
	}


	/**
	 * Perform the search requested from the HTTP form post.
	 */
	function doSearchAction()
	{
		// retrieve the search results by building a search criteria
		$itemMgr =& ItemTypeManager::instance();

		//	build criteria to describe
		//	(
		//		SellerDBID == playerID OR
		//		(
		//			ItemType == search_type_name AND
		//			BidRange( search_min_bid, search_max_bid )
		//		)
		//	)

		// do inner AND grouped criteria first

		$searchCriteria = '';
		// search item type name
		if (isset( $_REQUEST['search_type_name'] ) and
				( $searchTypeName = $_REQUEST['search_type_name'] ) )
		{
			$itemTypesList = Array();
			foreach( $itemMgr->itemTypes as $itemType => $itemTypeInfo )
			{

				if (eregi( $searchTypeName, $itemTypeInfo->name ))
				{
					$itemTypesList[] = $itemType;
				}
			}

			try
			{
				$res = $this->auctionHouse->webCreateItemTypeCriteria( array(
					   "itemTypes" => $itemTypesList ) );
			}
			catch( Exception $e )
			{
				$this->addExceptionMsgAndDisconnect( $e );
				return;
			}
			$itemTypeCriteria = $res[ "criteria" ];

			if ($searchCriteria == FALSE)
			{
				$searchCriteria = $itemTypeCriteria;
			}
			else
			{
				try
				{
					$res = $this->auctionHouse->webCombineAnd( array(
						"criteria1" => $searchCriteria,
						"criteria2" => $itemTypeCriteria ) );
					$searchCriteria = $res["criteria"];
				}
				catch( Exception $e )
				{
					$this->addExceptionMsg( $e );
					return;
				}
			}
		}
		// search min - max bid
		$searchMinBid = -1;
		$searchMaxBid = -1;
		if (isset( $_REQUEST['search_min_bid'] ))
		{
			if ($_REQUEST['search_min_bid'] == '')
			{
				$searchMinBid = -1;
			}
			else
			{
				$searchMinBid = (int)$_REQUEST['search_min_bid'];
			}
		}
		if (isset( $_REQUEST['search_max_bid'] ))
		{
			if ($_REQUEST['search_max_bid'] == '')
			{
				$searchMaxBid = -1;
			}
			else
			{
				$searchMaxBid = (int)$_REQUEST['search_max_bid'];
			}
		}
		if ($searchMinBid != -1 or $searchMaxBid != -1)
		{
			try
			{
				$res = $this->auctionHouse->webCreateBidRangeCriteria( array(
					"minBid" => $searchMinBid,
					"maxBid" => $searchMaxBid ) );
			}
			catch( Exception $e )
			{
				$this->addExceptionMsg( $e );
				return;
			}

			$bidRangeCriteria = $res["criteria"];
			if ($searchCriteria == FALSE)
			{
				$searchCriteria = $bidRangeCriteria;
			}
			else
			{
				try
				{
					$res = $this->auctionHouse->webCombineAnd( array(
						"criteria1" => $searchCriteria,
						"criteria2" => $bidRangeCriteria ) );
					$searchCriteria = $res["criteria"];
				}
				catch( Exception $e )
				{
					$this->addExceptionMsg( $e );
					return;
				}
			}
		}

		try
		{
			$res = $this->auctionHouse->webCreateBidderCriteria( array(
				"dbID" => (int)$this->playerDBID ) );
		}
		catch( Exception $e )
		{
			$this->addExceptionMsg( $e );
			return;
		}

		$bidderCriteria = $res["criteria"];

		if (!$searchCriteria)
		{
			if (isset( $_REQUEST['search_submit'] ))
			{
				// retrieve all
				// leave search criteria empty
			}
			else
			{
				// only retrieve auctions player has bid on
				$searchCriteria = $bidderCriteria;
			}
		}
		else // if ($searchCriteria)
		{
			// encapsulate with seller criteria if the result of the
			// above is not empty,otherwise we want ALL auctions (ie
			// the player hit search without specifying criteria)
			try
			{
				$res = $this->auctionHouse->webCombineOr( array(
					"criteria1" => $bidderCriteria,
					"criteria2" => $searchCriteria ) );
				$searchCriteria = $res["criteria"];
			}
			catch( Exception $e )
			{
				$this->addExceptionMsg( $e );
				return;
			}
		}

		// perform the search
		try
		{
			$res = $this->auctionHouse->webSearchAuctions( array(
				"criteria" => $searchCriteria ) );
		}
		catch( Exception $e )
		{
			$this->addExceptionMsg( $e );
			// TODO: On severe errors
			// $this->addExceptionMsgAndDisconnect( $e );
			return;
		}

		debug( "searchCriteria: $searchCriteria" );
		debugObj( $res );

		// retrieve the info for each returned auction ID
		try
		{
			$res = $this->auctionHouse->webGetAuctionInfo( array(
				"auctions" => $res["searchedAuctions"] ) );
		}
		catch( Exception $e )
		{
			$this->addExceptionMsg( $e );
			return;
		}

		$this->searchResults = array();

		foreach ($res[ 'auctionInfo' ] as $auctionInfo)
		{
			// If current bid is buyout price, don't include in result, as it
			// has already been bought out
			if ($auctionInfo[ 'currentBid' ] == $auctionInfo[ 'buyoutPrice' ])
			{
				continue;
			}

			// If there is no time remaining, don't display the auction.
			// Results may linger for a couple of seconds.
			$res = $this->auctionHouse->webGetTime();
			$this->officialTime = $res["time"];
			If ($auctionInfo[ 'expiry' ] < $this->officialTime)
			{
				continue;
			}

			$auctionInfo['item'] = Array(
				$auctionInfo['itemLock'],
				$auctionInfo['itemType'],
				$auctionInfo['itemSerial']
			);
			$this->searchResults[$auctionInfo['auctionID']] = $auctionInfo;

		}

		debug( "search results: \n". debugStringObj( $this->searchResults ) );

		// retrieve the seller names as we go
		$sellerNames = Array();

		// move all the player's bids to $this->playerBids
		$this->playerBids = Array();

		foreach (array_keys( $this->searchResults ) as $auctionID)
		{
			$auctionInfo =& $this->searchResults[$auctionID];
			$sellerDBID = $auctionInfo['sellerDBID'];

			if (!isset( $sellerNames[$sellerDBID] ))
			{
				$entity = new RemoteEntity(
					"entities_by_id/Avatar/" . $auctionInfo[ "sellerDBID" ] );

				try
				{
					$res = $entity->webGetPlayerInfo();
					$sellerNames[$sellerDBID] = $res[ 'playerName' ];
				}
				catch( Exception $e )
				{
					debug( "Could not get seller info for:\n" .
							debugStringObj( $auctionInfo ) . " - " .
							$this->addExceptionMsg( $e ) );
					return;
				}
			}

			$sellerName = $sellerNames[ $sellerDBID ];

			$auctionInfo['playerName'] = $sellerName;
			if (in_array( $this->playerDBID, $auctionInfo['bidders'] ))
			{
				$this->playerBids[ $auctionID ] = $auctionInfo;
				unset( $this->searchResults[$auctionID] );
			}
			else if ($this->playerDBID == $auctionInfo['sellerDBID'] )
			{
				unset( $this->searchResults[$auctionID] );
			}
		}
	} //  function doSearchAction()


	function renderHead()
	{
		AuthenticatedXHTMLMpPage::renderHead();

		// JavaScript for setting default bid amounts and enabling/disabling
		// buyout and bid submit buttons
		?>
<script language="JavaScript">

var selectedAuctionItemName = '';
var selectedAuctionBuyoutAmount = -1;

function updateBid( recommendedBid, buyoutAmount, itemName )
{
	bidAmountTextInput = document.getElementById( 'bid_amount' );
	if (bidAmountTextInput.value == '' )
	{
		// disable for now
		// bidAmountTextInput.value = recommendedBid;
	}

	document.getElementById( 'bid_submit' ).disabled = false;
	document.getElementById( 'buyout_submit' ).disabled = (buyoutAmount == null);

	selectedAuctionItemName = itemName;
	selectedAuctionBuyoutAmount = buyoutAmount;

}
</script>
<?php
	}


	function initialise()
	{
		// set up class variables accessed later on
		$this->player = $this->auth->entity();

		$this->auctionHouse =
				new RemoteEntity( "global_entities/AuctionHouse" );

		try
		{
			$res = $this->player->webGetPlayerInfo();
		}
		catch( Exception $e )
		{
			// error - should log out
			$this->addExceptionMsgAndDisconnect( $e );
			return;
		}

		$this->playerDBID = $res["databaseID"];

		$res = $this->player->webGetGoldAndInventory();
		$this->availableGold = $res["goldPieces"];

		$action = FALSE;

		// handle form action based on submit button name
		// of the form action:(.+)
		foreach( array_keys( $_REQUEST ) as $key)
		{
			$matches = Array();
			if (ereg( 'action:(.+)', $key, $matches ))
			{
				$action = $matches[1];
			}
		}

		if ($action)
		{
			$redirect = 'SearchAuctions.php?';

			// add search parameters that resulted in this action
			$firstSearchParam = TRUE;
			foreach(
				Array(
					"search_type_name",
					"search_min_bid",
					"search_max_bid",
					"search_submit"
				) as $searchParam
			)
			{
				if (!$firstSearchParam)
				{
					$redirect .= "&";
				}
				$redirect .= urlencode( $searchParam ). '='.
					urlencode( $_REQUEST[$searchParam] );
				$firstSearchParam = FALSE;
			}


			switch($action)
			{
				case "bid":
				case "buyout":
				{
					$auctionID = $_REQUEST['auction_select'];

					if (!$auctionID)
					{
						$this->addErrorMsg( "Selected auction is invalid." );
					}
					if ($action == "bid")
					{
						$this->doBidAction( $auctionID );
					}
					else // if ($action == "buyout")
					{
						$this->doBuyoutAction( $auctionID );
					}
				} break;

				default: 		break;
			}

			if ($redirect)
			{
				$this->setRedirect( $redirect );
				return;
			}
		}

		$this->doSearchAction();


		try
		{
			$res = $this->auctionHouse->webGetTime();
			$this->officialTime = $res["time"];
		}
		catch( exception $e )
		{
			$this->addExceptionMsgAndDisconnect( $e );
			return;
		}
	}

	function renderBody()
	{
		$this->renderErrorMsgs();
		$this->renderStatusMsgs();

		$this->printSearchForm();

		//echo xhtmlMpHeading( 'Official time ', 2 ). xhtmlMpPara(
		//	sprintf( "%.01f", $this->officialTime ) );

		$form = new XHTMLMpForm();
		$formContents = '';

		$searchResultsRows = $this->getSearchResultsRows();
		$playerBidRows = $this->getPlayerBidsRows();
		if ($searchResultsRows)
		{
			$formContents =
				xhtmlMpHeading( 'Search Results', 2 ).
				xhtmlMpTable( $searchResultsRows,
					'0', '', 'bidTable'
				);
		}
		else if (isset( $_REQUEST['search_submit'] ))
		{
			$formContents =
				xhtmlMpHeading( 'Search Results', 2 ).
				xhtmlMpPara( "No auctions found to match your criteria." );
		}


		$formContents .=
			xhtmlMpHeading( 'My Bids', 2 );
		if (!count( $this->playerBids ))
		{
			$formContents .= xhtmlMpPara(
				"You have made no bids. Search for items ".
				"you wish to bid for, and either enter a maximum bid or, for ".
				"nominated auctions, you may buy them out completely for the ".
				"buyout price."
			);

		}
		else
		{
			$formContents .= xhtmlMpTable(
				$playerBidRows,
				'0', '', 'tradeTable' );
		}

		$formContents .=
			xhtmlMpHeading( "Make Bid", 2 ).
			xhtmlMpTable(
				xhtmlMpTableRow(
					xhtmlMpTableCell(
						"Maximum bid: ".
						xhtmlMpFormInputText(
							'bid_amount', '', '3', '', ' id="bid_amount"'
						).
						' / '.
						xhtmlMpTag( 'b', $this->availableGold. " gold available" )
					).
					xhtmlMpTableCell(
						xhtmlMpFormSubmit(
							'action:bid', 'Bid', '',
							' disabled="true" id="bid_submit"'
						)
					)
				).
				xhtmlMpTableRow(
					xhtmlMpTableCell( '&nbsp;' ).
					xhtmlMpTableCell(
						xhtmlMpFormSubmit( 'action:buyout', 'Buyout', '',
							' disabled="true" id="buyout_submit" '.
							' onClick="return confirm( '.
								'\'Are you sure you wish to buyout this '.
								'auction of \' + '.
								'selectedAuctionItemName + '.
								'\' for \' + '.
								'selectedAuctionBuyoutAmount + '.
								'\' gold?\');" '
						)
					)
				),
				'0', '50%',
				'auctionActions'
			);

		$form->setContents( $formContents );
		// add the search criteria so that the redirection goes back to the set of
		// searched auctions
		$form->addHidden( 'search_type_name', $_REQUEST['search_type_name'] );
		$form->addHidden( 'search_min_bid', $_REQUEST['search_min_bid'] );
		$form->addHidden( 'search_max_bid', $_REQUEST['search_max_bid'] );

		echo( $form->output() );

	}

	function printSearchForm()
	{
		echo xhtmlMpHeading( 'Search Criteria', 2 );
		$form = new XHTMLMPForm();
		$form->setContents(
			xhtmlMpPara(
				'Item Type:&nbsp;'.
					xhtmlMpFormInputText( 'search_type_name',
						isset( $_REQUEST['search_type_name'] ) ?
							$_REQUEST['search_type_name'] : '',
						8 ).
				' Bid Range:&nbsp;'.
					xhtmlMpFormInputText( 'search_min_bid',
						isset( $_REQUEST['search_min_bid'] ) ?
							$_REQUEST['search_min_bid'] : '',
						3 ).
					'&nbsp;-&nbsp;'.
					xhtmlMpFormInputText( 'search_max_bid',
						isset( $_REQUEST['search_max_bid'] ) ?
							$_REQUEST['search_max_bid'] : '',
						3 ).
				' '.
				xhtmlMpFormSubmit( 'search_submit', 'Search' )
			)
		);

		$form->setClassName( 'auctionSearchForm' );
		$form->setMethod( 'get' );
		echo $form->output();


	}

	function getSearchResultsRows()
	{
		if (!$this->searchResults)
		{
			return;
		}

		$itemMgr =& ItemTypeManager::instance();


		$searchResultRows = xhtmlMpTableRow(
			xhtmlMpTableHeader( 'Item', '', ' colspan="2"').
			xhtmlMpTableHeader( 'Seller' ).
			xhtmlMpTableHeader( 'Expiry' ).
			xhtmlMpTableHeader( 'Current bid' ).
			xhtmlMpTableHeader( 'Buyout price' )
		);

		$sellerNames = Array();

		$first = '-first';
		$count = 0;
		foreach ($this->searchResults as $auctionID => $auctionInfo)
		{
			list( $itemLock, $itemType, $itemSerial ) = $auctionInfo['item'];
			$itemTypeName = $itemMgr->itemTypes[$itemType]->name;
			$itemTypeImgUrl = $itemMgr->itemTypes[$itemType]->iconImgUrl;
			$currentBid = $auctionInfo['currentBid'];
			$recommendedBid = (int) min( $currentBid + 1, 1.05 * $currentBid );

			$onClickJS = 'onClick="updateBid( '.
				$recommendedBid. ", ".
				(is_null($auctionInfo['buyoutPrice']) ?
						'null' : $auctionInfo['buyoutPrice']).
					", ".
				"'". $itemTypeName. "' );".
				'return true;"'
				;

			$cells =
				xhtmlMpTableCell(
					xhtmlMpFormInput( 'radio', 'auction_select', $auctionID,
						'', ' '.
						(($currentBid > $this->availableGold) ? 'disabled="true" ':'' ).
						$onClickJS ),
					'0'
				).
				xhtmlMpTableCell(
					xhtmlMpImg( $itemTypeImgUrl, '' ).
					$itemTypeName
				).
				xhtmlMpTableCell( $auctionInfo['playerName'] ).
				xhtmlMpTableCell(
//					sprintf( "%.01f",
//						$auctionInfo['expiry'] - $this->officialTime
//					).
//					" [".
					timeFloatToString(
						$auctionInfo['expiry'] - $this->officialTime )
//					."]"

				).
				xhtmlMpTableCell(
					xhtmlMpSpan( $auctionInfo['currentBid'],
					(($currentBid > $this->availableGold) ?
						"auctionBad" : "auctionGood" )
					)
				);

			if (is_null( $auctionInfo['buyoutPrice'] ))
			{
				$cells .= xhtmlMpTableCell( '-' );
			}
			else
			{
				$cells .= xhtmlMpTableCell( $auctionInfo['buyoutPrice'] );
			}


			$searchResultRows .= xhtmlMpTableRow(
				$cells,
				"alt". (($count++ % 2) + 1). $first
			);

			$first = FALSE;
		}

		if (!$count)
		{
			$searchResultRows .= xhtmlMpTableRow(
				xhtmlMpTableCell( "No auctions found.", FALSE, '', ' colspan="7"' ),
				'alt1-first'
			);
		}

		return $searchResultRows;

	}


	function getPlayerBidsRows()
	{
		$rows = xhtmlMpTableRow(
			xhtmlMpTableHeader( "Item", '', ' colspan="2"' ).
			xhtmlMpTableHeader( "Seller" ).
			xhtmlMpTableHeader( "Expiry" ).
			xhtmlMpTableHeader( "Current bid" ).
			xhtmlMpTableHeader( "Buyout price" ).
			xhtmlMpTableHeader( "Status" )
		);

		$itemMgr =& ItemTypeManager::instance();

		$first = "-first";
		$count = 0;
		foreach ($this->playerBids as $auctionID => $auctionInfo )
		{
			list( $itemLock, $itemType, $itemSerial ) = $auctionInfo['item'];
			$itemTypeName = $itemMgr->itemTypes[$itemType]->name;
			$itemTypeImgUrl = $itemMgr->itemTypes[$itemType]->iconImgUrl;

			$currentBid = $auctionInfo['currentBid'];
			$recommendedBid = (int) min( $currentBid + 1, 1.05 * $currentBid );

			$onClickJS = 'onClick="updateBid( '.
				$recommendedBid. ', '.
				(is_null($auctionInfo['buyoutPrice']) ?
						'null' : $auctionInfo['buyoutPrice']).
					', '.
				"'". $itemTypeName. "' ); return true;\" ";
				;

			$cells =
				xhtmlMpTableCell(
					xhtmlMpFormInput( 'radio', 'auction_select', $auctionID,
						'', " $onClickJS" ),
					'0'
				).
				xhtmlMpTableCell(
					xhtmlMpImg( $itemTypeImgUrl, $itemTypeName ).
					$itemTypeName
				).
				xhtmlMpTableCell(
					$auctionInfo['playerName']
				).
				xhtmlMpTableCell(
					timeFloatToString( $auctionInfo['expiry'] -
						$this->officialTime )
				).
				xhtmlMpTableCell( $auctionInfo['currentBid']  );
			if (!is_null( $auctionInfo['buyoutPrice'] ))
			{
				$cells .= xhtmlMpTableCell( $auctionInfo['buyoutPrice'] );
			}
			else
			{
				$cells .= xhtmlMpTableCell( '&nbsp;' );
			}
			if ($auctionInfo['highestBidder'] == $this->playerDBID)
			{
				$cells .= xhtmlMpTableCell(
					xhtmlMpSpan( "You are the highest bidder", "auctionGood" ).
					xhtmlMpSingleTag( 'br' ).
					"Max bid: ". $auctionInfo['currentMaxBid']
				);
			}
			else if (in_array( $this->playerDBID, $auctionInfo['bidders'] ))
			{
				$cells .= xhtmlMpTableCell(
					xhtmlMpSpan( "You have been outbid", 'auctionBad' )
				);
			}
			else
			{
				$cells .= xhtmlMpTableCell( "&nbsp;" );
			}

			$rows .= xhtmlMpTableRow(
				$cells,
				"alt". (($count++ % 2) + 1). $first
			);
			$first = "";
		}

		return $rows;
	}

}
// ----------------------------------------------------------------------------
// Section: Main body
// ----------------------------------------------------------------------------

$page = new SearchAuctionsPage();
$page->render();
?>
