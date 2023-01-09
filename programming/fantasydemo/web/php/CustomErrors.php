<?php
require_once( "BWError.php" );

class BWCustomError extends BWError {}
class BWPHPError extends BWError {}

// Custom errors. These declarations match the error classes in
// fantasydemo/res/scripts/server_common/CustomErrors.py
class AuctionHouseError extends BWCustomError {}
class BidError extends BWCustomError {}
class BuyoutError extends BWCustomError {}
class CreateEntityError extends BWCustomError {}
class DBError extends BWCustomError {}
class InsufficientGoldError extends BWCustomError {}
class InvalidAuctionError extends BWCustomError {}
class InvalidItemError extends BWCustomError {}
class ItemLockError extends BWCustomError {}
class PriceError extends BWCustomError {}
class SearchCriteriaError extends BWCustomError {}

// Errors specific to PHP
class NoConnectionError extends BWPHPError {}
class NoSuchURLError extends BWPHPError {}
class InvalidFieldError extends BWPHPError {}
?>
