<?php
require_once( 'AuthenticatedPage.php' );
require_once( 'Constants.php' );
require_once( 'ItemViewer.php' );
require_once( 'Items.php' );

class InventoryPage extends AuthenticatedXHTMLMPPage
{
	var $inventory; 		// BW result
	var $itemsOrder; 		// order of serials
	var $items; 			// serials mapped to item description arrays

	var $databaseID; 		// database ID of the current player
	var $viewItemSerial;	// whether we are viewing a particular item ID

	function InventoryPage()
	{
		AuthenticatedXHTMLMPPage::AuthenticatedXHTMLMPPage( "Inventory",
			"BWAuthenticator" );

		$this->inventory = FALSE;
		$this->itemsOrder = Array();
		$this->items = Array();

		$this->viewItemSerial = FALSE;
	}


	function initialise()
	{
		$itemMgr =& ItemTypeManager::instance();
		$entity = $this->auth->entity();

		if (isset( $_REQUEST['view_item'] ))
		{
			$this->viewItemSerial = $_REQUEST['view_item'];
		}

		try
		{
			$res = $entity->webGetGoldAndInventory();
		}
		catch( Exception $e )
		{
			$this->addExceptionMsgAndDisconnect( $e );
			return;
		}

		// debug("webGetGoldAndInventory res\n". debugStringObj($res));

		$this->inventory = array( 'goldPieces' => $res["goldPieces"],
				'inventoryItems' => $res["inventoryItems"],
				'lockedItems' => $res["lockedItems"] );
		// debug("webGetGoldAndInventory\n". debugStringObj($this->inventory));

		foreach ($this->inventory['inventoryItems'] as $item)
		{
			$this->itemsOrder[] = $item['serial'];
			$this->items[$item['serial']] = $item;
		}

		# find the action
		foreach( array_keys( $_REQUEST ) as $key )
		{
			$matches = Array();
			if (ereg( 'action:(.*)', $key, $matches ))
			{
				$_REQUEST['action'] = $matches[1];
			}
		}

		if (isset( $_REQUEST['action'] ))
		{
			switch ($_REQUEST['action'])
			{
				case "auction":
				{
					if (!isset( $_REQUEST['item_select'] ))
					{
						$this->addStatusMsg(
							'No item selected for auctioning.' );
						return;
					}

					$itemSerialToAuction = (int)$_REQUEST['item_select'];


					if (!isset( $_REQUEST['auction_bid_price'] ) or
						($bidPrice = (int)$_REQUEST['auction_bid_price']) <= 0)
					{
						$this->addErrorMsg( 'Invalid starting bid price.' );
						return;
					}
					if (!isset( $_REQUEST['auction_buyout'] ) or
						($buyout = (int)$_REQUEST['auction_buyout']) <= 0 )
					{
						$buyout = -1;
					}

					if (!isset( $_REQUEST['auction_expiry'] ) or
						($expiry = (int)$_REQUEST['auction_expiry']) <= 0)
					{
						$this->addErrorMsg( 'Invalid expiry time.' );
						return;
					}

					try
					{
						$res = $entity->webCreateAuction( array(
							"itemSerial" => $itemSerialToAuction,
							"expiry" => $expiry,
							"startBid" => $bidPrice,
							"buyout" => $buyout ) );
					}
					catch( Exception $e )
					{
						$this->addExceptionMsg( $e );
						return;
					}
					$auctionId = $res["auctionID"];

					$itemType = $this->items[$itemSerialToAuction]['itemType'];
					$itemName = $itemMgr->itemTypes[$itemType]->name;
					$this->addStatusMsg( "Auction of $itemName created." );
					$this->setRedirect( "PlayerAuctions.php?auction_id=" .
						urlencode( $auctionId ) );
				}
				return;

				default:
					break;
			}
		}


	}


	function renderBody()
	{
		if ($this->inventory == FALSE)
		{
			$this->addErrorMsg( "Could not get inventory list." );
			return;
		}

		$itemViewer = FALSE;
		if ($this->viewItemSerial !== FALSE)
		{
			$itemViewer = new ItemViewer( $this->viewItemSerial );
			$itemType = $this->items[$this->viewItemSerial]['itemType'];
			$itemViewer->acquireInfo( $itemType );
		}


		$this->renderErrorMsgs();
		$this->renderStatusMsgs();

		$formContents = '';

		echo(
			xhtmlMpDiv(
				'Gold: '. $this->inventory['goldPieces'],
				'goldRow'
			)
		);

		$rows = '';
		$rowNum = 0;

		$first = '-first';

		$itemMgr =& ItemTypeManager::instance();

		foreach ($this->itemsOrder as $itemIndex => $itemSerial)
		{
			if ($this->items[$itemSerial]['lockHandle'] != -1)
			{
				// this is a locked item
				continue;
			}
			$itemType =& $this->items[$itemSerial]['itemType'];
			$itemTypeInfo =& $itemMgr->itemTypes[$itemType];

			$cells =
				xhtmlMpTableCell(
					xhtmlMpImg( $itemTypeInfo->iconImgUrl, '' ),
					FALSE, '',
					' width="25"' ).
				xhtmlMpTableCell(
					xhtmlMpFormInputRadio( "item_select",
						$itemSerial,
						isset($_REQUEST['item_select']) ?
							$_REQUEST['item_select'] == $itemSerial : FALSE
					),
					FALSE, '',
					' width="24"'
				).
				xhtmlMpTableCell(
					xhtmlMpLink(
						"Inventory.php?view_item=$itemSerial#item-detail",
						$itemTypeInfo->name
					)
				);

			$rows .= xhtmlMpTableRow( $cells, 'alt'.
				(string)($rowNum % 2 + 1).$first );

			++$rowNum;
			$first = '';
		}
		$formContents .= xhtmlMpTable( $rows, 0, '', 'inventory',
			' cellspacing="0" cellpadding="0"' );

		// old trading example
		//$formContents .= xhtmlMpDiv(
		//	xhtmlMpFormSubmit( 'action:sell', 'Sell Items', 'sellButton' ),
		//	'sellButton' );

		$formContents .= xhtmlMpHeading( "Create Auction", 2 );

		$formContents .= xhtmlMpDiv(
			xhtmlMpTable(
				xhtmlMpTableRow(
					xhtmlMpTableCell( "Bid price: " ).
					xhtmlMpTableCell(
						xhtmlMpFormInputText( 'auction_bid_price',
							isset( $_REQUEST['auction_bid_price'] ) ?
								$_REQUEST['auction_bid_price'] : '0',
							2 ).
							'gold'
					)
				).
				xhtmlMpTableRow(
					xhtmlMpTableCell(
						"Buyout price (optional): "
					).
					xhtmlMpTableCell(
						xhtmlMpFormInputText( 'auction_buyout',
							isset( $_REQUEST['auction_buyout'] ) ?
								$_REQUEST['auction_buyout'] : '0',
							2 ).
						'gold'
					)
				).
				xhtmlMpTableRow(
					xhtmlMpTableCell( "Expiry: " ).
					xhtmlMpTableCell(
						xhtmlMpFormInputRadioList( 'auction_expiry',
							Array(
								'60' => '1 min',
								'300' => '5 min',
								'3600' => '1 hour',
								'86400' => '1 day'
							),
							isset( $_REQUEST['auction_expiry'] ) ?
								$_REQUEST['auction_expiry'] : '3600'
							// default to 3600 seconds for now
						)
					)
				).
				xhtmlMpTableRow(
					xhtmlMpTableCell(
						xhtmlMpFormSubmit( 'action:auction', 'Create Auction',
							'sellButton' )
					)
				), 0, '',
				'inventoryActions'
			)
		);


		echo( xhtmlMpTag( 'a', '', '', ' name="top"' ) );

		if ($rowNum == 0)
		{
			echo xhtmlMpDiv( "Inventory is empty.", 'status' );
		}
		else
		{
			$form = new XHTMLMPForm();
			$form->setAction( "Inventory.php" );
			$form->setMethod( "post" );
			$form->setContents( $formContents );
			echo( $form->output() );
		}

		if ($itemViewer)
		{
			echo( $itemViewer->output() );
		}

	}

}

$page = new InventoryPage();
$page->render();
?>
