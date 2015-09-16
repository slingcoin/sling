// Copyright (c) 2014 The Sling developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "rpcserver.h"
#include "market.h"
#include "smessage.h"
#include "init.h" // pwalletMain

#include <boost/lexical_cast.hpp>
#include <boost/assign/list_of.hpp>

#include <json_spirit_value.h>

#include <QWidget>

using namespace json_spirit;

//parameters: none
//example: marketalllistings
Value marketalllistings(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error("marketalllistings \n."
                            "List of all active market listings.");

    Array ret;

    LOCK(cs_markets);

    BOOST_FOREACH(PAIRTYPE(const uint256, CSignedMarketListing)& p, mapListings)
    {
        // only show if no buy requests
        bool bFound = false;
        CSignedMarketListing item = p.second;

        BOOST_FOREACH(PAIRTYPE(const uint256, CBuyRequest)& b, mapBuyRequests)
        {
            if(b.second.listingId == item.GetHash())
            {
                if(b.second.nStatus != LISTED || b.second.nStatus != BUY_REQUESTED)
                {
                    bFound = true;
                }
            }
        }

        if(bFound)
            continue; // go to next item, this one someone is already buying

        Object obj;
        std::string titleItem = item.listing.sTitle;

        if (titleItem != "")
        {
            std::string vendorItemId = CBitcoinAddress(item.listing.sellerKey.GetID()).ToString();
            std::string priceItem = QString::number(item.listing.nPrice / COIN, 'f', 8).toStdString();
            std::string idItem = item.GetHash().ToString();
            std::string expireItem = DateTimeStrFormat(item.listing.nCreated + (7 * 24 * 60 * 60));

            obj.push_back(Pair("vendorItemId", vendorItemId));
            obj.push_back(Pair("priceItem", priceItem));
            obj.push_back(Pair("titleItem", titleItem));
            obj.push_back(Pair("idItem", idItem));
            obj.push_back(Pair("expireItem", expireItem));

            ret.push_back(obj);
        }

    }

    return ret;
}

//parameters: search hammers
//example: marketsearchlistings aveng
Value marketsearchlistings(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 0)
        throw runtime_error("marketalllistings \n."
                            "List of all active market listings.");

    string strSearch = params[0].get_str();
    std::transform(strSearch.begin(), strSearch.end(),strSearch.begin(), ::tolower);

    Array ret;

    LOCK(cs_markets);

    BOOST_FOREACH(PAIRTYPE(const uint256, CSignedMarketListing)& p, mapListings)
    {
        // only show if no buy requests
        bool bFound = false;
        CSignedMarketListing item = p.second;

        BOOST_FOREACH(PAIRTYPE(const uint256, CBuyRequest)& b, mapBuyRequests)
        {
            if(b.second.listingId == item.GetHash())
            {
                if(b.second.nStatus != LISTED || b.second.nStatus != BUY_REQUESTED)
                {
                    bFound = true;
                }
            }
        }

        if(bFound)
            continue; // go to next item, this one someone is already buying

        std::string titleItem = item.listing.sTitle;
        std::transform(titleItem.begin(), titleItem.end(), titleItem.begin(), ::tolower);

        int foundSearch = titleItem.find(strSearch);

        if (foundSearch != -1 && titleItem != "")
        {
            Object obj;
            std::string vendorItemId = CBitcoinAddress(item.listing.sellerKey.GetID()).ToString();
            std::string priceItem = QString::number(item.listing.nPrice / COIN, 'f', 8).toStdString();
            std::string idItem = item.GetHash().ToString();
            std::string expireItem = DateTimeStrFormat(item.listing.nCreated + (7 * 24 * 60 * 60));

            obj.push_back(Pair("vendorItemId", vendorItemId));
            obj.push_back(Pair("priceItem", priceItem));
            obj.push_back(Pair("titleItem", titleItem));
            obj.push_back(Pair("idItem", idItem));
            obj.push_back(Pair("expireItem", expireItem));

            ret.push_back(obj);
        }

    }

    return ret;
}

//parameters: ListingID
//example: marketbuy d6560d158876e79043be5202844e7b7fa01839894f7251d1056d5a349f946cbe
Value marketbuy(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 0)
        throw runtime_error("marketbuy \n.""Buys a market listing.");

    string itemID = params[0].get_str();
    uint256 idHash = uint256(itemID);

    CBuyRequest buyRequest;
    buyRequest.buyerKey = pwalletMain->GenerateNewKey();
    buyRequest.listingId = idHash;
    buyRequest.nStatus = BUY_REQUESTED;
    buyRequest.requestId = buyRequest.GetHash();
    SignBuyRequest(buyRequest, buyRequest.vchSig);
    ReceiveBuyRequest(buyRequest);
    buyRequest.BroadcastToAll();

    return Value::null;;
}

//parameters: ListingID RequestID
//example: marketapprovebuy d6560d158876e79043be5202844e7b7fa01839894f7251d1056d5a349f946cbe c6dad3be5202844e7b7fa01839894f7251d1056d5a349f946cbe
Value marketapprovebuy(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 2)
        throw runtime_error("marketapprovebuy \n."
                            "Approves market listing buy request.");

    string itemID = params[0].get_str();
    string requestID = params[1].get_str();

    uint256 idListingHash = uint256(itemID);
    uint256 idRequestHash = uint256(requestID);

    CBuyAccept accept;
    accept.listingId = idListingHash;
    accept.buyRequestId = idRequestHash;
    accept.nDate = GetTime();
    accept.sellerKey = pwalletMain->GenerateNewKey();

    CBuyRequest buyRequest = mapBuyRequests[idRequestHash];
    // create the escrow lock address
    std::string errors;
    std::string escrowAddress;

    AddMultisigAddress(mapListings[idListingHash].listing.sellerKey, mapBuyRequests[idRequestHash].buyerKey, escrowAddress, errors);
    accept.escrowAddress = escrowAddress;

    // fund it
    std::string strError = "";
    CWalletTx wtxNew;

    CreateEscrowLockTx(accept.escrowAddress, mapListings[buyRequest.listingId].listing.nPrice + (0.01 * COIN), strError, wtxNew);

    // serialize the tx to a string
    CDataStream ssTx(SER_NETWORK, CLIENT_VERSION);
    ssTx.reserve(sizeof(wtxNew));
        ssTx << wtxNew;

    // misuse this parameter like a boss
    accept.raw = ssTx.str();

    SignBuyAccept(accept, accept.vchSig);
    ReceiveBuyAccept(accept);
    accept.BroadcastToAll();

    return Value::null;;
}

//parameters: ListingID RequestID
//example: marketrejectbuy d6560d158876e79043be5202844e7b7fa01839894f7251d1056d5a349f946cbe c6dad3be5202844e7b7fa01839894f7251d1056d5a349f946cbe
Value marketrejectbuy(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 2)
        throw runtime_error("marketrejectbuy \n."
                            "Rejects market listing buy request.");

    string itemID = params[0].get_str();
    string requestID = params[1].get_str();

    uint256 idListingHash = uint256(itemID);
    uint256 idRequestHash = uint256(requestID);

    CBuyReject reject;
    reject.listingId = idListingHash;
    reject.buyRequestId = idRequestHash;
    reject.nDate = GetTime();
    reject.sellerKey = pwalletMain->GenerateNewKey();
    SignBuyReject(reject, reject.vchSig);
    ReceiveBuyReject(reject);
    reject.BroadcastToAll();

    return Value::null;;
}

//parameters: none
//example: marketbuyrequests
Value marketbuyrequests(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 0)
        throw runtime_error("marketbuyrequests \n."
                            "Returns your market buy requests.");
    return Value::null;;
}

//parameters: none
//example: marketmylistings
Value marketmylistings(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 0)
        throw runtime_error("marketmylistings \n."
                            "Returns your market listings.");
    return Value::null;;
}

//parameters: {JSON}
Value marketsell(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error("marketsell \n.""Creates a market listing.");

    Object result;

    return result;
}
