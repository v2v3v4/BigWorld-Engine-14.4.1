<?php

require_once( 'XHTML-MP-functions.php' );

$itemTypeManagerInstance = new ItemTypeManager();



class ItemType
{
	var $id;
	var $name;
	var $stats;
	var $description;
	var $iconImgUrl;
	var $detailImgUrl;

	function ItemType( $id, $name, $stats, $description, $iconImgUrl,
			$detailImgUrl )
	{
		$this->id 			= $id;
		$this->name 		= $name;
		$this->stats		= $stats;
		$this->description 	= $description;
		$this->iconImgUrl	= $iconImgUrl;
		$this->detailImgUrl	= $detailImgUrl;
	}

	function render()
	{
		$statTableRows = '';

		foreach ($this->stats as $name => $value )
		{
			$statTableRows .= xhtmlMpTableRow(
				xhtmlMpTableHeader( $name ).
				xhtmlMpTableCell( $value )
			);
		}

		echo( xhtmlMpDiv(
			xhtmlMpDiv( xhtmlMpImg( $this->detailImgUrl, $this->name ),
					'item-detail-image' ).
				xhtmlMpHeading( $this->name, 'item-detail-name' ).
				xhtmlMpTable( $statTableRows, 0, '',
					'item-detail-description' ).
				xhtmlMpDiv( $this->description, 'item-detail-description' ),
			'item-detail'
		) );
	}
}


class ItemTypeManager
{
	var $itemTypes;
	function ItemTypeManager()
	{
		$this->itemTypes = Array();
	}


	static function &instance()
	{
		global $itemTypeManagerInstance;
		return $itemTypeManagerInstance;
	}

	function registerItemType( $itemType )
	{
		$this->itemTypes[$itemType->id] = $itemType;
	}

	function renderItemType( $itemType )
	{
		if (in_array( $itemType, $this->itemTypes ))
		{
			$this->itemTypes[$itemType]->render();
			return TRUE;
		}
		else
		{
			return FALSE;
		}

	}

}

$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_STAFF,
		'Staff of Serpents',
		Array(
			'Magic damage'	=>	'50',
			'Durability'	=>	'100',
		),
		'Staff imbued with the spirit of the snake.',
		'images/items/icon_snake_staff.gif',
		'images/items/detail_snake_staff.jpg'
	)
);


$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_STAFF_2,
		'Staff of Lightning',
		Array(
			'Magic damage'	=>	'50',
			'Durability'	=>	'100',
		),
		'Staff imbued with the electrical essence.',
		'images/items/icon_lightning_staff.gif',
		'images/items/detail_lightning_staff.jpg'
	)
);


$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_DRUMSTICK,
		'Drumstick',
		Array(
			'Health bonus'	=>	'+20',
		),
		'Increases health.',
		'images/items/icon_drumstick.gif',
		'images/items/detail_drumstick.jpg'
	)
);


$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_SPIDER_LEG,
		'Spider leg',
		Array(
			'Durability'	=>	'100',
		),
		'A leg from a giant spider.',
		'images/items/icon_spiderleg.gif',
		'images/items/detail_spiderleg.jpg'
	)
);

$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_BINOCULARS,
		'Binoculars',
		Array(
		),
		'Allows you to see into the distance.',
		'images/items/icon_binoculars1.gif',
		'images/items/detail_binoculars1.jpg'
	)
);

$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_SWORD,
		'Sword',
		Array(
			'Accuracy'	=>	'80%',
			'Speed'		=>	'3s',
			'Power'		=>	'40',
			'Type'		=>	'melee'
		),
		'An elegant weapon for a civilised age. '.
			'Perfect for dueling mercenaries.',
		'images/items/icon_sword2.gif',
		'images/items/detail_sword2.jpg'
	)
);


$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_SWORD_2,
		'Bastard Sword',
		Array(
			'Accuracy'	=>	'75%',
			'Speed'		=>	'3.5s',
			'Power'		=>	'60',
			'Type'		=>	'melee'
		),
		'A (slightly larger) elegant weapon for a civilised age. '.
			'Perfect for dueling mercenaries.',
		'images/items/icon_sword3.gif',
		'images/items/detail_sword3.jpg'
	)
);


$itemTypeManagerInstance->registerItemType(
	new ItemType(
		BW_ITEM_TYPE_GOBLET,
		'Goblet',
		Array(
			'Health bonus'	=>	'+20',
		),
		'Healing water.',
		'images/items/icon_goblet2.gif',
		'images/items/detail_goblet2.jpg'
	)
);
