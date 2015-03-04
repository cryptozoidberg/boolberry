/**
 * Created by roky on 03/03/15.
 */


/************* from native to javascript API (callbacks) ***************/
//this functions called periodically and show daemon and wallet states

function on_update_daemon_state(param){};
/*
params =  {
    "daemon_network_state": 2,
        "hashrate": 0,
        "height": 9729,
        "inc_connections_count": 0,
        "last_blocks": [
    {
        "date": 1425441268,
        "diff": "107458354441446",
        "h": 9728,
        "type": "PoS"
    },{
        "date": 1425441256,
        "diff": "2778612",
        "h": 9727,
        "type": "PoW"
    }],
        "last_build_available": "0.0.0.0",
        "last_build_displaymode": 0,
        "max_net_seen_height": 9726,
        "out_connections_count": 2,
        "pos_difficulty": "107285151137540",
        "pow_difficulty": "2759454",
        "synchronization_start_height": 9725,
        "text_state": "Online"
}
*/

function on_update_wallet_status(param){}
/*
params =  {

    "wallet_state": 2
}
*/

function on_update_wallet_info(param){}
/*
param = {
    "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ",
        "balance": 20605413534000000,
        "do_mint": 1,
        "mint_is_in_progress": 0,
        "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_2.lui",
        "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f",
        "unlocked_balance": 20553902633000000
}
*/
function on_money_transfered(param){}

/*
 param = {
    "balance": 20743454806000000,
        "ti": {
        "amount": 17255039000000,
            "fee": 0,
            "height": 9752,
            "is_income": true,
            "payment_id": "",
            "recipient": "",
            "recipient_alias": "",
            "td": {
            "rcv": [9000000,30000000,5000000000,50000000000,200000000000,7000000000000,10000000000000,20000000000000000],
                "spn": [20000000000000000]
        },
        "timestamp": 1425444971,
            "tx_blob_size": 424,
            "tx_hash": "1841e26b571261a4e221ffa865a23abf87db9c57a4eb9770f41f4731cf03d7be",
            "unlock_time": 9762
    },
    "unlocked_balance": 626199767000000
}

*/

//main function which called back with result, after every API called

function dispatch(status, param)
{
    /*
    status = {
        request_id: "124",
        error_code: "OK" //OK success, other - error codes depends of API.
                         //it's gonna be text constants, to make sense in case of html have no idea about code
                         //e.g.: "BUSY", "INTERNAL_ERROR", "WRONG_PASSWORD" e.t.c.
    };


    param = {}; //whatever API wants to return
    */
}




/************* from javascript to native API ***************/

/* Every API call immediately return object with following fields:

var api_response = {
    error_code: "OK",
    request_id: "234AB3"
}

In case of request successfully queued in dispatch que request_id contains string id, that consists of numbers and probably letters
Error code could be one ot the text constants: "OK", "QUE_FULL", "INTERNAL_ERROR", "BUSY" etc
*/


/* PAYMENTS API */
function open_wallet(params){}
/*

params = {
    path: "/full/wallet/file/path.lui",
    pass: "12345"
}

If you need the way to show Open File Dialog just le me know

response = {
    wallet_id: "1234",

}



*/


function generate_wallet(){}
/*

 params = {
    path: "/full/wallet/file/path.lui",
    pass: "12345"
 }

 If you need the way to show Save File Dialog just le me know
*/



function get_recent_transfers(params){}
/*

params = {
   wallet_id: "2324",
   amount: 100,
   offset: 0
}

response = {
    "history": [{
    "amount": 17255368000000,
    "fee": 0,
    "height": 9732,
    "is_income": true,
    "payment_id": "",
    "recipient": "",
    "recipient_alias": "",
    "td": {
        "rcv": [8000000,60000000,300000000,5000000000,50000000000,200000000000,4000000000000,20000000000000],
        "spn": [7000000000000]
    },
    "timestamp": 1425441352,
    "tx_blob_size": 420,
    "tx_hash": "609e84c6493788016e2834e7d22c9b0efb985b2357f01fe9ee4f5556345ebe28",
    "unlock_time": 9742
},{
    "amount": 17255385000000,
    "fee": 0,
    "height": 9731,
    "is_income": true,
    "payment_id": "",
    "recipient": "",
    "recipient_alias": "",
    "td": {
        "rcv": [5000000,80000000,300000000,5000000000,50000000000,200000000000,7000000000000,20000000000000],
        "spn": [10000000000000]
    },
    "timestamp": 1425441332,
    "tx_blob_size": 421,
    "tx_hash": "5fca82613576d57487b82dd258e780752d0e25b1b7a21e2c2fde05b1cc25511b",
    "unlock_time": 9741
}]
}
*/

function close_wallet(){}

/*
params = {
    wallet_id: "2324",
}


response =
{}

*/


function transfer(params){}

/*
 params = {
    "wallet_id": "12345",
    "destinations":[
        {"address":"HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ","amount":"10.0"}],
    "mixin_count":0,
    "lock_time":0,
    "payment_id":"",
    "fee":"0.001000000000"
}

response = {
    "success": true,
    "tx_blob_size": 348,
    "tx_hash": "3b81e446ecfe6bd98146f58ad4519b94af8e70f238458d90315bf9e5291249dc"
}

*/

/* SYSTEM API*/
function get_version(){}
/*

response = "1.2.3 ";

*/


/* EXCHANGE API */


function push_offer(){}
function get_all_offers(){} //just for first time, in debug purposes
//TODO: add more exhcange API
