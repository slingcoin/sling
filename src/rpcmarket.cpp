// Copyright (c) 2014 The Sling developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "rpcserver.h"

#include <boost/lexical_cast.hpp>

#include "smessage.h"
#include "init.h" // pwalletMain

using namespace json_spirit;
using namespace std;

//extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);


Value marketalllistings(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error("marketalllistings \n.""List of all active market listings.");

    Object result;

    return result;
}

Value marketsearchlistings(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error("marketsearchlistings \n.""Search all active market listings.");

    Object result;

    return result;
}

Value marketbuy(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error("marketbuy \n.""Buys a market listing.");

    Object result;

    return result;
}

Value marketsell(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error("marketsell \n.""Creates a market listing.");

    Object result;

    return result;
}
