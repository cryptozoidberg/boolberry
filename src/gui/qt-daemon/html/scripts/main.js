/// <reference path="jquery-2.1.1.js" />

function initialize() {
    console.log("initialize()");

    
    Qt.update_daemon_state.connect(str_to_obj.bind({ cb: on_update_daemon_state }));
    Qt.update_wallet_status.connect(str_to_obj.bind({ cb: on_update_wallet_status }));
    Qt.update_wallet_info.connect(str_to_obj.bind({ cb: on_update_wallet_info }));
    Qt.money_transfer.connect(str_to_obj.bind({ cb: on_money_transfer }));

    Qt.show_wallet.connect(show_wallet);
    Qt.hide_wallet.connect(hide_wallet);
    Qt.set_recent_transfers.connect(str_to_obj.bind({ cb: on_set_recent_transfers }));
    //  Qt.handle_internal_callback.connect(on_handle_internal_callback);
    
    // left menu clicks
    $('#walletitem').on('click', onWalletItemClick);
    $('#openwalletitem').on('click', onOpenWalletItemClick);
    $('#createwalletitem').on('click', onCreateWalletItemClick);
    $('#restorewalletitem').on('click', onRestoreWalletItemClick);
    $('#transactionsitem').on('click', onTransactionsItemClick);
    $('#addressbookitem').on('click', onAddressBookItemClick);

    // openwallet screen clicks
    $('#openwalletbtn').on('click', on_open_wallet);
    $('#browsewalletbtn').on('click', on_browse_wallet_for_open);

    //createwallet screen clicks
    $('#createwalletcreatebtn').on('click', on_create_wallet);
    $('#createwalletbrowsebtn').on('click', on_browse_wallet_for_create);

    //restorewallet screen clicks
    $('#restorewalletcreatebtn').on('click', on_restore_wallet);
    $('#restorewalletbrowsebtn').on('click', on_browse_wallet_for_restore);

    // wallet screen clicks
    $('#walletclosebtn').on('click', on_close_wallet);
    $('#walletcopybtn').on('click', on_wallet_copy_addr);
    $('#walletsendaddress').on('focusin', on_wallet_send_address_focus);
    $('#walletsendamount').on('focusin', on_wallet_send_amount_focus);
    $('#walletsendpaymentid').on('focusin', on_wallet_send_paymentid_focus);
    $('#walletsendmorebtn').on('click', on_wallet_send_more);
    $('#walletsendbtn').on('click', on_wallet_send);
    $('#walletsendaddressbookbtn').on('click', on_wallet_send_book_btn);
    $('#walletsendaddresslist li').on('click', on_wallet_send_addresslist_click);

    // addressbook screen clicks
    $('#addressbookaddbtn').on('click', on_addrbook_add);
    $('#addressbooklist .copybtn').on('click', on_address_copy_btn);

    $('.infocellmoreicon').on('click', on_trx_more_details);

    fill_addressbook();
}

function onWalletItemClick() {
    console.log("onWalletItemClick()");
    $('.menuitem, .walletmenuitem, .screen').removeAttr('data-state');
    $('#walletitem, #wallet').attr('data-state', 'active');
    $('#walletmenu').attr('data-state', 'active');
    $("#walletsendresult").html('');
}

function onOpenWalletItemClick() {
    console.log("onOpenWalletItemClick()");
    $('.menuitem, .walletmenuitem, .screen, #walletmenu').removeAttr('data-state');
    $('#openwalletitem, #openwallet').attr('data-state', 'active');
    $("#walletsendresult").html('');
}

function onCreateWalletItemClick() {
    console.log("onCreateWalletItemClick()");
    $('.menuitem, .walletmenuitem, .screen, #walletmenu').removeAttr('data-state');
    $('#createwalletitem, #createwallet').attr('data-state', 'active');
    $("#walletsendresult").html('');
}

function onRestoreWalletItemClick() {
    console.log("onRestoreWalletItemClick()");
    $('.menuitem, .walletmenuitem, .screen, #walletmenu').removeAttr('data-state');
    $('#restorewalletitem, #restorewallet').attr('data-state', 'active');
    $("#walletsendresult").html('');
}

function onTransactionsItemClick() {
    console.log("onTransactionsItemClick()");
    $('.menuitem, .walletmenuitem, .screen').removeAttr('data-state');
    $('#walletitem').attr('data-state', 'active');
    $('#walletmenu').attr('data-state', 'active');
    $('#transactionsitem, #transactions').attr('data-state', 'active');
    $("#walletsendresult").html('');
}

function onAddressBookItemClick() {
    console.log("onAddressBookItemClick()");
    $('.menuitem, .walletmenuitem, .screen, #walletmenu').removeAttr('data-state');
    $('#addressbookitem, #addressbook').attr('data-state', 'active');
    $("#walletsendresult").html('');
}

function str_to_obj(str) {
 //   console.log("str_to_obj(" + str + ")");
    var info_obj = jQuery.parseJSON(str);
    this.cb(info_obj);
}

function build_prefix(count) {
    var result = "";
    for (var i = 0; i != count; i++)
        result += "0";

    return result;
}


function print_money(amount) {
    console.log("print_money(" + amount + ")");
    var sign = (amount >= 0);
    var val = amount;
    if (!sign)
        val = -amount;
    var am_str = val.toString();
    if (am_str.length <= 12) {
        am_str = build_prefix(13 - am_str.length) + am_str;
    }
    am_str = am_str.slice(0, am_str.length - 12) + "." + am_str.slice(am_str.length - 12);
    if (!sign)
        am_str = "-" + am_str;
    return am_str;
}


function on_handle_internal_callback(obj_str, callback_name) {
    console.log("on_handle_internal_callback: " + callback_name + ":" + obj_str);
    var obj = undefined;
    if (obj_str !== undefined && obj_str !== "") {
        obj = $.parseJSON(obj_str);
    }

    if (window[callback_name] === undefined) {
        console.log("callback \"" + callback_name + "\" not dound");
        return;
    }

    window[callback_name](obj);
}

function parse_and_get_locktime() {
    var lock_time = $('#walletsendunlocktime').val();
    if (lock_time === "")
        return 0;
    var items = lock_time.split(" ");
    if (!items.length)
        return 0;

    var blocks_count = 0;
    for (var i = 0; i != items.length; i++) {
        var multiplier = 1;
        if (items[i][items[i].length - 1] === "d") {
            items[i] = items[i].substr(0, items[i].length - 1);
            multiplier = 720;
        } else if (items[i][items[i].length - 1] === "h") {
            items[i] = items[i].substr(0, items[i].length - 1);
            multiplier = 30;
        }
        var value = parseInt(items[i]);
        if (!(value > 0)) {
            return undefined;
        }
        blocks_count += value * multiplier;
    }
    return blocks_count;
}

function on_wallet_send() {
    console.log("on_wallet_send()");
    var transfer_obj = {
        destinations: [
            {
                address: $('#walletsendaddress').val(),
                amount: $('#walletsendamount').val()
            }
        ],
        mixin_count: $('walletsendmixin').val(),
        lock_time: 0,
        fee:0,
        payment_id: ''
    };

    if ($('#walletsendpaymentid').val() == 'PAYMENT ID...')
        $('#walletsendpaymentid').val('');
    transfer_obj.payment_id = $('#walletsendpaymentid').val();
    var lock_time = parse_and_get_locktime();
    if (lock_time === undefined) {
        Qt_parent.message_box("Wrong transaction lock time specified.");
        return;
    }
    transfer_obj.lock_time = lock_time;
    transfer_obj.fee = $('#walletsendfee').val();

    var transfer_res_str = Qt_parent.transfer(JSON.stringify(transfer_obj));
    var transfer_res_obj = jQuery.parseJSON(transfer_res_str);

    console.log(JSON.stringify(transfer_res_obj));
    if (transfer_res_obj.success) {
        $('#walletsendaddress').val("ADDRESS...");
        $('#walletsendamount').val("AMOUNT...");
        $('#walletsendpaymentid').val("PAYMENT ID...");
        $('#walletsendmixin').val(1);
        $("#walletsendresult").html("<span style='color: #1b9700'> Money successfully sent, transaction " + transfer_res_obj.tx_hash + ", " + transfer_res_obj.tx_blob_size + " bytes</span><br>");
    }
    else
        return;
}


function on_set_recent_transfers(o) {
    console.log("on_set_recent_transfers(" + JSON.stringify(o) + ")");

    if (o === undefined)
        return;

    var str = "";
    var shortStr = "";
    var counter = 0;

    console.log("start unconfirmed");

    if (Array.isArray(o.unconfirmed)) {
        for (var i = 0; i < o.unconfirmed.length; i++) {
            str += get_trx_table_entry(o.unconfirmed[i], false);
            counter++;
            if (counter <= 3)
                shortStr = str;
        }
    }
    console.log("start history");

    if (Array.isArray(o.history)) {
        for (var i = 0; i < o.history.length; i++) {
            str += get_trx_table_entry(o.history[i], true);
            counter++;
            if (counter <= 3)
                shortStr = str;
        }
    }

    console.log("end history");
    console.log(str);
    console.log(shortStr);
    $("#wallettrxlist").html(shortStr);
    $("#transactionstrxlist").html(str);

    $('.infocellmoreicon').on('click', on_trx_more_details);

    console.log("end");
}


function on_close_wallet() {
    console.log("on_close_wallet()");
    Qt_parent.close_wallet();
}

function on_generate_new_wallet() {
    console.log("on_generate_new_wallet()");
    Qt_parent.generate_wallet();
}


function on_update_wallet_info(wal_status) {
    console.log("on_update_wallet_info(" + JSON.stringify(wal_status) + ")");
    $("#balanceview .informervalue").text(print_money(wal_status.balance));
    $("#unlockedview .informervalue").text(print_money(wal_status.unlocked_balance));
    $("#unconfirmedview .informervalue").text(print_money(wal_status.unconfirmed_balance));
    $("#walletaddress").attr("value", wal_status.address);
    $("#walletbalance").text("BALANCE:" + print_money(wal_status.balance));
/*    $("#wallet_address").text(wal_status.address);
    $("#wallet_path").text(wal_status.path);
    if (current_exchange_rate && current_exchange_rate !== undefined && current_exchange_rate !== 0) {
        $("#est_value_btc_id").text(((wal_status.unlocked_balance / 1000000000000) * current_exchange_rate).toFixed(5));
        if (current_btc_to_usd_exchange_rate && current_btc_to_usd_exchange_rate !== undefined && current_btc_to_usd_exchange_rate !== 0) {
            $("#est_value_usd_id").text((((wal_status.unlocked_balance / 1000000000000) * current_exchange_rate) * current_btc_to_usd_exchange_rate).toFixed(2));
        }
    }
    */
}

function on_money_transfer(tei) {
    console.log("on_money_transfer(" + JSON.stringify(tei) + ")");
    var tr_html = get_trx_table_entry(tei.ti, (tei.ti.height > 0));
    console.log('new row: ' + tr_html);
    // remove old row for this transaction
    console.log('remove old trx entries');
    var id = "tr[data-trxid='" + tei.ti.tx_hash + "']";
    console.log('id=' + id);
    console.log('Found: ' + $(id).length);
    if($(id).length > 0)
        $(id).remove();
    console.log('removed');
    // add row with new info for this transaction
    console.log('prepend rows');
    $("#transactionstrxlist").prepend(tr_html);
    $('#wallettrxlist').prepend(tr_html);
    console.log('set wallet balance');
    $("#wallet_balance").text(print_money(tei.balance));
    $("#wallet_unlocked_balance").text(print_money(tei.unlocked_balance));

    $('.infocellmoreicon').on('click', on_trx_more_details);

    console.log('finished');
}

function on_open_wallet() {
    console.log("on_open_wallet()");
    Qt_parent.open_wallet($('#openwalletpath').val(),
        $('#openwalletpwd').val());
    $('#walletrestoreseed').text('');
    $('#openwalletpwd').attr('value', '');
}

function on_create_wallet() {
    console.log("on_create_wallet()");
    if ($('#createwalletpwd').val() != $('#createwalletpwd2').val()) {
        alert("Re-typed password doesn't match.");
        return;
    }
    var restore_seed = Qt_parent.generate_wallet($('#createwalletname').val(),
        $('#createwalletpwd').val(), $('#createwalletpath').val());
    $('#walletrestoreseed').html("Use this words to restore this wallet in future:<br>" + restore_seed);
    $('#createwalletname').val('');
    $('#createwalletpwd').val('');
    $('#createwalletpwd2').val('');
}

function on_restore_wallet() {
    console.log("on_restore_wallet()");
    if ($('#restorewalletpwd1').val() != $('#restorewalletpwd2').val()) {
        alert("Re-typed password doesn't match.");
        return;
    }
    Qt_parent.restore_wallet($('#restorewalletseed').val(),
        $('#restorewalletpwd1').val(), $('#restorewalletpath').val());
    $('#walletrestoreseed').text('');
    $('#restorewalletpwd1').val('');
    $('#restorewalletpwd2').val('');
    $('#restorewalletseed').val('');
}

function on_browse_wallet_for_open() {
    console.log("on_browse_wallet_for_open()");
    var path = Qt_parent.browse_wallet(true);
    $('#openwalletpath').attr('value', path);
}

function on_browse_wallet_for_restore() {
    console.log("on_browse_wallet_for_restore()");
    var path = Qt_parent.browse_wallet(false);
    $('#restorewalletpath').attr('value', path);
    console.log("path=" + path);
}

function on_browse_wallet_for_create() {
    console.log("on_browse_wallet_for_create()");
    var path = Qt_parent.browse_wallet(false);
    $('#createwalletpath').attr('value', path);
}

function on_update_wallet_status(wal_status) {
    console.log("on_update_wallet_status(" + JSON.stringify(wal_status) + ")");
    if (wal_status.wallet_state == 1) {
        //syncmode
    } else if (wal_status.wallet_state == 2) {
        if ($('#walletitem').hasClass('is-disabled')) {
            onWalletItemClick();
            $('#walletitem').removeClass('is-disabled');
        }
    }
    else {
        alert("wrong state");
    }
}

function on_update_daemon_state(info_obj) {
//    console.log("on_update_daemon_state(" + JSON.stringify(info_obj) + ")");
    if (info_obj.daemon_network_state == 0)//daemon_network_state_connecting
    {
        $("#connectionstatusimage").attr("src", "images/statedisconnected.png");
        $("#progresstext").text("");
    } else if (info_obj.daemon_network_state == 1)//daemon_network_state_synchronizing
    {
        $("#connectionstatusimage").attr("src", "images/stateconnected.png");
        var percents = 0;
        if (info_obj.max_net_seen_height > info_obj.synchronization_start_height)
            percents = ((info_obj.height - info_obj.synchronization_start_height) * 100) / (info_obj.max_net_seen_height - info_obj.synchronization_start_height);
        $("#blockchainprogress").attr("value", percents);
        $("#progresstext").text("Downloading blockchain...");
    } else if (info_obj.daemon_network_state == 2)//daemon_network_state_online
    {
       if( ($('#wait').attr('data-state') == 'active') ||
            ($('#openwalletitem').hasClass('is-disabled') && $('#walletitem').hasClass('is-disabled'))) {
            $('#openwalletitem, #createwalletitem, #restorewalletitem').removeClass('is-disabled');
            $("#blockchainprogress").attr("value", 100);
            $("#progresstext").text("");
            $('#wait').removeAttr('data-state');
            onOpenWalletItemClick();
        }
        $("#connectionstatusimage").attr("src", "images/stateconnected.png");
        $("#blockchainprogress").attr("value", percents);
        $("#progresstext").text("Synchronized");
    } else {
        $("#connectionstatusimage").attr("src", "images/statedisconnected.png");
    }

    $("#connectstatedescr").text(info_obj.text_state);

/*    $("#daemon_out_connections_text").text(info_obj.out_connections_count.toString());
    if (info_obj.out_connections_count >= 8) {
        $("#daemon_out_connections_text").addClass("daemon_view_general_status_value_success_text");
    }*/
}

function show_wallet() {
    console.log("show_wallet()");
    if ($('#walletitem').hasClass('is-disabled')) {
        onWalletItemClick();
        $('#walletitem').removeClass('is-disabled');
    }
}

function hide_wallet() {
    console.log("hide_wallet()");
    $("#balanceview .informervalue").text("0.000000000000");
    $("#unlockedview .informervalue").text("0.000000000000");
    $("#walletaddress").attr("value", "");
    $("#walletbalance").text("BALANCE: 0.000000000000");
    onOpenWalletItemClick();
    $('#walletitem').addClass('is-disabled');
    $('#walletrestoreseed').text('');
}

function on_wallet_copy_addr() {
    console.log("on_wallet_copy_addr()");
    Qt_parent.place_to_clipboard($('#walletaddress').val());
}

function on_addrbook_add() {
    console.log("on_addrbook_add");
    if ($('#addressbookname').val().length < 1) {
        alert("Name shouldn't be empty.");
        $('#addressbookname').focus();
        return;
    }
    if ($('#addressbookaddress').val().length < 1) {
        alert("Address shouldn't be empty.");
        $('#addressbookaddress').focus();
        return;
    }
    Qt_parent.add_address($('#addressbookname').val(),
        $('#addressbookaddress').val(), $('#addressbookalias').val());
    $('#addressbookname').val('');
    $('#addressbookaddress').val('');
    $('#addressbookalias').val('');
    fill_addressbook();
}

function on_address_copy_btn() {
    alert($(this).attr('data-address'));
    Qt_parent.place_to_clipboard($(this).attr('data-address'));
}

function fill_addressbook() {
    console.log("fill_addressbook");
    var str = Qt_parent.get_addressbook();
    console.log(str);
    var ab = JSON.parse(str);
    var html = "";
    var list = "";
    if (Array.isArray(ab.entries)) {
        for (var i = 0; i < ab.entries.length; i++) {
            var entry = ab.entries[i];
            html += "<tr>";
            html += '<td><div class="tablelabel">' + entry.name + '&nbsp;</div></td>';
            html += '<td><input type="text" value="' + entry.address + '"/></td>';
            html += '<td><input class="copybtn" type="button" value="Copy" data-address="' + entry.address + '"/></td>';
            html += "</tr>";
            list += '<li data-address="' + entry.address + '">' + entry.name + '</li>';
        }
    }
    $('#addressbooklist').html(html);
    $('#walletsendaddresslist ul').html(list);

    $('#addressbooklist .copybtn').on('click', on_address_copy_btn);
    $('#walletsendaddresslist li').on('click', on_wallet_send_addresslist_click);
}

function on_wallet_send_address_focus() {
    if ($('#walletsendaddress').val() != "ADDRESS...")
        return;
    $('#walletsendaddress').attr('value', '');
}

function on_wallet_send_amount_focus() {
    if ($('#walletsendamount').val() != "AMOUNT...")
        return;
    $('#walletsendamount').attr('value', '');
}

function on_wallet_send_paymentid_focus() {
    if ($('#walletsendpaymentid').val() != "PAYMENT ID...")
        return;
    $('#walletsendpaymentid').attr('value', '');
}

function on_wallet_send_more() {
    if ($(this).val() == "More") {
        $('#sendparams .hiddenparam').show();
        $(this).attr('value', "Less");
    } else {
        $('#sendparams .hiddenparam').hide();
        $(this).attr('value', "More");
    }
}

function on_wallet_send_book_btn() {
    console.log('on_wallet_send_book_btn');
    if ($('#walletsendaddresslist li').length < 1) {
        alert('Addressbook is empty.');
        return;
    }
    var position = $('#walletsendaddressbookbtn').offset();
    position.top += 30;
    $('#walletsendaddresslist').css(position)
    $('#walletsendaddresslist').show();
}


function on_wallet_send_addresslist_click() {
    console.log('on_wallet_send_addresslist_click()');
    $('#walletsendaddress').attr('value', $(this).attr('data-address'));
    $('#walletsendaddresslist').hide();
}

function on_trx_more_details() {
    console.log('on_trx_more_details');
    var info_block =  $(this).next('.trxdetails');
    if (info_block.is(':visible')) {
        info_block.hide();
        $(this).children('.infocellheader').text('MORE');
    }
    else {
        info_block.show();
        $(this).children('.infocellheader').text('LESS');
    }
}