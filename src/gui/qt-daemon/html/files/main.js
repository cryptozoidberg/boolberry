

function update_last_ver_view(mode)
{
    if(mode == 0)
        $('#last_actual_version_text').hide();
    else if(mode == 1)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_new");
    }else if(mode == 2)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_calm");
    }else if(mode == 3)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_urgent");
    }else if(mode == 4)
    {
        $('#last_actual_version_text').show();
        $('#last_actual_version_text').removeClass();
        $('#last_actual_version_text').addClass("daemon_view_param_value_last_ver_info_critical");
    }

}

var aliases_set = {};
var current_exchange_rate = undefined;
var current_btc_to_usd_exchange_rate = undefined;

function update_aliases_autocompletion()
{
    if(aliases_set.aliases)
        return;

    aliases_set = jQuery.parseJSON(Qt_parent.request_aliases());
    if(aliases_set.aliases)
    {
        console.log("aliases loaded: " + aliases_set.aliases.length);
        var availableTags = [];
        for(var i=0; i < aliases_set.aliases.length; i++)
        {
            availableTags.push("@" + aliases_set.aliases[i].alias);
        }
        $( "#transfer_address_id" ).autocomplete({
            source: availableTags,
            minLength: 2
        });
    }
    else
    {
        console.log("internal error: aliases  not loaded");
    }
}

function on_update_daemon_state(info_obj)
{
    //var info_obj = jQuery.parseJSON(daemon_info_str);

    if(info_obj.daemon_network_state == 0)//daemon_network_state_connecting
    {
        //do nothing
    }else if(info_obj.daemon_network_state == 1)//daemon_network_state_synchronizing
    {
        var percents = 0;
        if (info_obj.max_net_seen_height > info_obj.synchronization_start_height)
            percents = ((info_obj.height - info_obj.synchronization_start_height)*100)/(info_obj.max_net_seen_height - info_obj.synchronization_start_height);
        $("#synchronization_progressbar").progressbar( "option", "value", percents );
        if(info_obj.max_net_seen_height)
            $("#daemon_synchronization_text").text((info_obj.max_net_seen_height - info_obj.height).toString() + " blocks behind");
        $("#open_wallet_button").button("disable");
        $("#generate_wallet_button").button("disable");
        $("#restore_wallet_button").button("disable");
        $("#domining_button").button("disable");
        //show progress
    }else if(info_obj.daemon_network_state == 2)//daemon_network_state_online
    {
        $("#synchronization_progressbar_block").hide();
        $("#daemon_status_text").addClass("daemon_view_general_status_value_success_text");
        $("#open_wallet_button").button("enable");
        $("#generate_wallet_button").button("enable");
        $("#restore_wallet_button").button("enable");        
        disable_tab(document.getElementById('wallet_view_menu'), false);
        //load aliases
        update_aliases_autocompletion();
        //$("#domining_button").button("enable");
        //show OK
    }else if(info_obj.daemon_network_state == 3)//deinit
    {
        $("#synchronization_progressbar_block").show();
        $("#synchronization_progressbar" ).progressbar({value: false });
        $("#daemon_synchronization_text").text("");

        $("#daemon_status_text").removeClass("daemon_view_general_status_value_success_text");
        $("#open_wallet_button").button("disable");
        $("#generate_wallet_button").button("disable");
        $("#restore_wallet_button").button("disable");        
        //$("#domining_button").button("disable");

        hide_wallet();
        inline_menu_item_select(document.getElementById('daemon_state_view_menu'));
        disable_tab(document.getElementById('wallet_view_menu'), true);
        //show OK
    }else if(info_obj.daemon_network_state == 4)//deinit
    {
        $("#synchronization_progressbar_block").hide();
        $("#synchronization_progressbar" ).progressbar({value: false });
        $("#daemon_synchronization_text").text("");

        $("#daemon_status_text").removeClass("daemon_view_general_status_value_success_text");
        $("#daemon_status_text").addClass("daemon_view_general_status_value_fail_text");

        $("#open_wallet_button").button("disable");
        $("#generate_wallet_button").button("disable");
        $("#restore_wallet_button").button("disable");        
        //$("#domining_button").button("disable");

        hide_wallet();
        inline_menu_item_select(document.getElementById('daemon_state_view_menu'));
        disable_tab(document.getElementById('wallet_view_menu'), true);
        //show OK
    }

    else
    {
        //set unknown status
        $("#daemon_status_text").text("Unkonwn state");
        return;
    }

    $("#daemon_status_text").text(info_obj.text_state);

    $("#daemon_out_connections_text").text(info_obj.out_connections_count.toString());
    if(info_obj.out_connections_count >= 8)
    {
        $("#daemon_out_connections_text").addClass("daemon_view_general_status_value_success_text");
    }
    //$("#daemon_inc_connections_text").text(info_obj.inc_connections_count.toString());

    $("#daemon_height_text").text(info_obj.height.toString());
    $("#difficulty_text").text(info_obj.difficulty);
    $("#hashrate_text").text(info_obj.hashrate.toString());
    if(info_obj.last_build_displaymode < 3)
        $("#last_actual_version_text").text("(available version: " + info_obj.last_build_available + ")");
    else
        $("#last_actual_version_text").text("(Critical update: " + info_obj.last_build_available + ")");
    update_last_ver_view(info_obj.last_build_displaymode);
}

function on_update_wallet_status(wal_status)
{

    if(wal_status.wallet_state == 1)
    {
        //syncmode
        $("#transfer_button_id").button("disable");
        $("#synchronizing_wallet_block").show();
        $("#synchronized_wallet_block").hide();
    }else if(wal_status.wallet_state == 2)
    {
        $("#transfer_button_id").button("enable");
        $("#synchronizing_wallet_block").hide();
        $("#synchronized_wallet_block").show();
    }
    else
    {
        alert("wrong state");
    }
}

function build_prefix(count)
{
    var result = "";
    for(var i = 0; i != count; i++)
        result += "0";

    return result;
}

function print_money(amount)
{
    var am_str  = amount.toString();
    if(am_str.length <= 12)
    {
        am_str = build_prefix(13 - am_str.length) + am_str;
    }
    am_str = am_str.slice(0, am_str.length - 12) + "." + am_str.slice(am_str.length - 12);
    return am_str;
}



function get_details_block(td, div_id_str, transaction_id, blob_size, payment_id, fee, unlock_time)
{
    var res = "<div class='transfer_entry_line_details' id='" + div_id_str + "'> <span class='tx_details_text'>Transaction id:</span> " +  transaction_id
        + "<br><span class='tx_details_text'>size</span>: " + blob_size.toString()  + " bytes"
        +  "<br><span class='tx_details_text'>fee</span>: " + print_money(fee)
        + "<br><span class='tx_details_text'>unlock_time</span>: " + unlock_time
        +  "<br><span class='tx_details_text'>split transfers</span>: <br>";

    if(payment_id !== '' && payment_id !== undefined)
    {
        res += "<span class='tx_details_text'>Payment id:</span> " + "<span class='tx_details_value'>" + payment_id + "</span>" + "<br>";
    }

    if(td.rcv !== undefined)
    {
        for(var i=0; i < td.rcv.length; i++)
        {
            res += "<img class='transfer_text_img_span' src='files/income_ico.png' height='15px' /> " + print_money(td.rcv[i]) + "<br>";
        }
    }
    if(td.spn !== undefined)
    {
        for(var i=0; i < td.spn.length; i++)
        {
            res += "<img class='transfer_text_img_span' src='files/outcome_ico.png' height='15px' /> " + print_money(td.spn[i]) + "<br>";
        }
    }

    res += "</div>";
    return res;
}

// format implementation used from http://stackoverflow.com/questions/610406/javascript-equivalent-to-printf-string-format/4673436#4673436
// by Brad Larson, fearphage
if (!String.prototype.format) {
    String.prototype.format = function() {
        var args = arguments;
        return this.replace(/{(\d+)}/g, function(match, number) {
            return typeof args[number] != 'undefined'
                ? args[number]
                : match
                ;
        });
    };
}

function get_transfer_html_entry(tr, is_recent)
{
    var res;
    var color_str;
    var img_ref;
    var action_text;

    if(is_recent)
    {
        color_str = "#6c6c6c";
    }else
    {
        color_str = "#3a3a3a";
    }

    if(tr.height != 0)
    {
        if(tr.is_income)
        {
            img_ref = "files/income_ico.png";
            action_text = "Received";
        }
        else
        {
            img_ref = "files/outcome_ico.png";
            action_text = "Sent";
        }
    }else
    {
        if(tr.is_income)
        {
            img_ref = "files/unconfirmed_received.png"
        }
        else
        {
            img_ref = "files/unconfirmed_sent.png";
        }
        action_text = "Unconfirmed";
    }

    var dt = new Date(tr.timestamp*1000);

    var transfer_line_tamplate = "<div class='transfer_entry_line' id='transfer_entry_line_{4}_id' style='color: {0}'>";
    transfer_line_tamplate +=       "<img  class='transfer_text_img_span' src='{1}' height='15px'>";
    transfer_line_tamplate +=       "<span class='transfer_text_time_span'>{2}</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_status_span'>{6}</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_details_span'>";
    transfer_line_tamplate +=          "<a href='javascript:;' onclick=\"jQuery('#{4}_id').toggle('fast');\" class=\"options_link_text\"><img src='files/tx_icon.png' height='12px' style='padding-top: 4px'></a>";
    transfer_line_tamplate +=       "</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_amount_span'>{3}</span>";
    transfer_line_tamplate +=       "<span class='transfer_text_recipient_span' title='{7}'>{8}</span>";
    transfer_line_tamplate +=       "{5}";
    transfer_line_tamplate +=     "</div>";

    var short_string = tr.destination_alias.length ?  "@" + tr.destination_alias : (tr.destinations.substr(0, 8) + "..." +  tr.destinations.substr(tr.destinations.length - 8, 8) );
    transfer_line_tamplate = transfer_line_tamplate.format(color_str,
        img_ref,
        dt.format("yyyy-mm-dd HH:MM"),
        print_money(tr.amount),
        tr.tx_hash,
        get_details_block(tr.td, tr.tx_hash + "_id", tr.tx_hash, tr.tx_blob_size, tr.payment_id, tr.fee, tr.unlock_time ? tr.unlock_time - tr.height: 0),
        action_text,
        tr.destinations,
        short_string);

    return transfer_line_tamplate;
}

function on_update_wallet_info(wal_status)
{
    $("#wallet_balance").text(print_money(wal_status.balance));
    $("#wallet_unlocked_balance").text(print_money(wal_status.unlocked_balance));
    $("#wallet_address").text(wal_status.address);
    $("#wallet_path").text(wal_status.path);
    if(current_exchange_rate && current_exchange_rate !== undefined && current_exchange_rate !== 0)
    {
        $("#est_value_btc_id").text( ((wal_status.unlocked_balance/1000000000000 )*current_exchange_rate).toFixed(5));
        if(current_btc_to_usd_exchange_rate && current_btc_to_usd_exchange_rate !== undefined && current_btc_to_usd_exchange_rate !== 0)
        {
            $("#est_value_usd_id").text( (((wal_status.unlocked_balance/1000000000000 )*current_exchange_rate)*current_btc_to_usd_exchange_rate).toFixed(2));
        }
    }

}

function on_money_transfer(tei)
{
    var id = "#" + "transfer_entry_line_" + tei.ti.tx_hash +"_id";
    $(id).remove();
    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tei.ti, false));
    $("#wallet_balance").text(print_money(tei.balance));
    $("#wallet_unlocked_balance").text(print_money(tei.unlocked_balance));
}

function on_open_wallet()
{
    Qt_parent.open_wallet();
}

function on_generate_new_wallet()
{
  var seed_phrase = Qt_parent.generate_wallet();

  if (seed_phrase) {
    showModal(seed_phrase);
  }
}

function showModal(text) {
  var $modal = $('#modal');
  var $modalText = $('#modal-text');
  var $modalButton = $('#modal-button');

  $($modal).show();
  $($modalText).text(text);

  $($modalButton).click(function() {
    $($modalText).text('');
    $($modal).hide();
  });
}

function on_restore_wallet()
{
    var seed_phrase = Qt_parent.get_seed_text();
    if (seed_phrase.length === 0)
        return;

    var wallet_path = Qt_parent.browse_wallet(false);
    if (wallet_path.length === 0)
        return;

    var pass = Qt_parent.get_password();
    if (pass.length === 0)
        return;

    var r = Qt_parent.restore_wallet(seed_phrase, pass, wallet_path);
    if(!r) 
    {
        Qt_parent.message_box("Unable to restore wallet");
    }
}

function on_restore_wallet()
{
    var seed_phrase = Qt_parent.get_seed_text();
    if (seed_phrase.length === 0)
        return;

    var wallet_path = Qt_parent.browse_wallet(false);
    if (wallet_path.length === 0)
        return;

    var pass = Qt_parent.get_password();
    if (pass.length === 0)
        return;

    var r = Qt_parent.restore_wallet(seed_phrase, pass, wallet_path);
    if(!r) 
    {
        Qt_parent.message_box("Unable to restore wallet");
    }
}

function show_wallet()
{
    //disable_tab(document.getElementById('wallet_view_menu'), false);
    //inline_menu_item_select(document.getElementById('wallet_view_menu'));
    $("#wallet_workspace_area").show();
    $("#wallet_welcome_screen_area").hide();
    $("#recent_transfers_container_id").html("");
    $("#unconfirmed_transfers_container_id").html("");
}

function hide_wallet()
{
    $("#wallet_workspace_area").hide();
    $("#wallet_welcome_screen_area").show();
}

function on_switch_view(swi)
{
    if(swi.view === 1)
    {
        //switch to dashboard view
        inline_menu_item_select(document.getElementById('daemon_state_view_menu'));
    }else if(swi.view === 2)
    {
        //switch to wallet view
        inline_menu_item_select(document.getElementById('wallet_view_menu'));
    }else
    {
        Qt_parent.message_box("Wrong view at on_switch_view");
    }
}

function on_close_wallet()
{
    Qt_parent.close_wallet();
}


var last_timerId;

function test_parse_and_get_locktime_function()
{
    $('#lock_time').val("1d 2h");
    if(parse_and_get_locktime() !== 780)
    {
        alert("[parse_and_get_locktime]Test Failed");
        return;
    }

    $('#lock_time').val("1d 2h 22");
    if(parse_and_get_locktime() !== 802)
    {
        alert("[parse_and_get_locktime]Test Failed");
        return;
    }

    $('#lock_time').val("22");
    if(parse_and_get_locktime() !== 22)
    {
        alert("[parse_and_get_locktime]Test Failed");
        return;
    }

    $('#lock_time').val("d222");
    if(parse_and_get_locktime() !== undefined)
    {
        alert("[parse_and_get_locktime]Test Failed");
        return;
    }

    $('#lock_time').val("sfdd");
    if(parse_and_get_locktime() !== undefined)
    {
        alert("[parse_and_get_locktime]Test Failed");
        return;
    }

    $('#lock_time').val("3d 2h 34j");
    if(parse_and_get_locktime() !== undefined)
    {
        alert("[parse_and_get_locktime]Test Failed");
        return;
    }

}


function parse_and_get_locktime()
{
    var lock_time = $('#lock_time').val();
    if(lock_time === "")
        return 0;
    var items = lock_time.split(" ");
    if(!items.length )
        return 0;

    var blocks_count = 0;
    for(var i = 0; i!=items.length; i++)
    {
        var multiplier = 1;
        if(items[i][items[i].length-1] === "d")
        {
            items[i] = items[i].substr(0, items[i].length-1);
            multiplier = 720;
        }else if(items[i][items[i].length-1] === "h")
        {
            items[i] = items[i].substr(0, items[i].length-1);
            multiplier = 30;
        }
        var value = parseInt(items[i]);
        if( !(value > 0) )
        {
            return undefined;
        }
        blocks_count += value * multiplier;
    }
    return blocks_count;
}


function on_sign() {
	var sign_res_str = Qt_parent.sign_text($('#sign_text_id').val());
  var aign_res_obj = jQuery.parseJSON(sign_res_str);

  $('#signature_id').text(aign_res_obj.signature_hex);
  $('#signature_id').html("<span id='signature_id_text'>" + aign_res_obj.signature_hex + "</span>");

  if(last_timerId !== undefined) {
    clearTimeout(last_timerId);
  }

  $("#signature_id").show("fast");
  last_timerId = setTimeout(function(){$("#signature_id").hide("fast");}, 15000);
}

function isBlank(str)
 {
    return (!str || /^\s*$/.test(str));
}
function on_transfer()
{
    var addressInputVal = $('#transfer_address_id').val();
    var addressValidation = $.parseJSON(Qt_parent.parse_transfer_target(addressInputVal));
    var transfer_obj = {
        destinations:[
            {
                address: $('#transfer_address_id').val(),
                amount: $('#transfer_amount_id').val()
            }
        ],
        mixin_count: 0,
        lock_time: 0
    };

    if (!addressValidation.valid)
	{
		return;
	}

	if(isBlank(addressValidation.payment_id_hex))
	{
      transfer_obj.payment_id = $('#payment_id').val();
    }else
	{
	  //do not set, wallet code set all the necessary things 
	}

    var lock_time = 0; //parse_and_get_locktime();
    if(lock_time === undefined)
    {
        Qt_parent.message_box("Wrong transaction lock time specified.");
        return;
    }
    transfer_obj.lock_time = lock_time;
    transfer_obj.mixin_count = parseInt($('#mixin_count_id').val());
    transfer_obj.fee = $('#tx_fee').val();

    if(!(transfer_obj.mixin_count >= 0 && transfer_obj.mixin_count < 11))
    {
        Qt_parent.message_box("Wrong Mixin parameter value, please set values in range 0-10");
        return;
    }

    var transfer_res_str = Qt_parent.transfer(JSON.stringify(transfer_obj));
    var transfer_res_obj = jQuery.parseJSON(transfer_res_str);

    if(transfer_res_obj.success)
    {


        $('#transfer_address_id').val("");
        $('#transfer_amount_id').val("");
        $('#payment_id').val("");
        $('#payment_id').removeClass('is-disabled');
        $('#transfer_address_id_container').removeClass('integrated');
        $('#payment_id').prop('disabled', false);
        $('#mixin_count_id').val(0);
        $("#transfer_result_span").html("<span style='color: #1b9700'> Money successfully sent, transaction " + transfer_res_obj.tx_hash + ", " + transfer_res_obj.tx_blob_size + " bytes</span><br>");
        if(last_timerId !== undefined)
            clearTimeout(last_timerId);

        $("#transfer_result_zone").show("fast");
        last_timerId = setTimeout(function(){$("#transfer_result_zone").hide("fast");}, 15000);
        hide_address_status();
    }
    else
        return;
}

function on_set_recent_transfers(o)
{
    if(o === undefined || o.history === undefined || o.history.length === undefined)
        return;

    var str = "";
    for(var i=0; i < o.history.length; i++)
    {
        str += get_transfer_html_entry(o.history[i], true);
    }
    $("#recent_transfers_container_id").prepend(str);

    str = "";
    for(var i=0; i < o.unconfirmed.length; i++)
    {
        str += get_transfer_html_entry(o.unconfirmed[i], false);
    }
    $("#unconfirmed_transfers_container_id").prepend(str);
}


function secure_request_url_result_handler(info_obj)
{
    //debug case
    if(info_obj === undefined)
        return;

    console.log("https://blockchain.info/ticker response:" + JSON.stringify(info_obj));

    current_btc_to_usd_exchange_rate = parseFloat(info_obj.last);
    console.log("Exchange rate BTC: " + current_btc_to_usd_exchange_rate.toFixed(8) + " USD");

}

function on_handle_internal_callback(obj_str, callback_name)
{
    //console.log("on_handle_internal_callback: " + callback_name + ":" + obj_str);
    var obj = undefined;
    if(obj_str !== undefined && obj_str !== "")
    {
        obj = $.parseJSON(obj_str);
    }

    if(window[callback_name] === undefined)
    {
        console.log("callback \"" + callback_name + "\" not dound");
        return;
    }

    window[callback_name](obj);
}



function init_btc_exchange_rate()
{
    req_arams = {
        method: "GET"
    };

    secure_request_url("https://www.bitstamp.net/api/ticker/", JSON.stringify(req_arams), "secure_request_url_result_handler");
    console.log("Request to https://www.bits222tamp.net/api/ticker/ sent");
}



function str_to_obj(str)
{
    var info_obj = jQuery.parseJSON(str);
    this.cb(info_obj);
}

// address validation
function on_address_change() {
    var addressInputVal =  $('#transfer_address_id').val();

    // not show status if address field empty
    if (addressInputVal === '') {
      hide_address_status();
      return;
    }

    var addressValidation = $.parseJSON(Qt_parent.parse_transfer_target(addressInputVal));

    if (addressValidation.valid) {
        if (addressValidation.payment_id_hex.length) {
            $('#transfer_address_id_container').addClass('integrated');
            $('#payment_id').val(addressValidation.payment_id_hex);
            $('#payment_id').addClass('is-disabled');
            $('#payment_id').prop('disabled', true);
            show_address_status('integrated');
        } else {
            show_address_status('valid');
          $('#payment_id').val('').removeClass('is-disabled');
          $('#payment_id').prop('disabled', false);
        }
    } else {
        show_address_status('invalid');
        $('#payment_id').val('').removeClass('is-disabled');
        $('#payment_id').prop('disabled', false);
    }
}

function show_address_status(status) {
    var newStatus = 'with-status' + ' ' + status;
    $('.address-status').removeClass('with-status valid invalid integrated');
    $('#transfer_address_id').removeClass('with-status');

    $('.address-status').addClass(newStatus);
    $('#transfer_address_id').addClass('with-status');

    switch (status) {
      case 'valid':
        $('.address-status-text').text('Valid address');
        break;
      case 'invalid':
        $('.address-status-text').text('Invalid address');
        break;
      case 'integrated':
        $('.address-status-text').text('Integrated address');
        break;
      default:
        break;
    }
}

function hide_address_status() {
    $('.address-status').removeClass('with-status valid invalid integrated');
    $('#transfer_address_id').removeClass('with-status');
    $('.address-status-text').text('');
}

$(function()
{ // DOM ready
    $( "#synchronization_progressbar" ).progressbar({value: false });
    $( "#wallet_progressbar" ).progressbar({value: false });
    $(".common_button").button();

    $("#open_wallet_button").button("disable");
    $("#generate_wallet_button").button("disable");
    $("#restore_wallet_button").button("disable");
    $("#domining_button").button("disable");


    //$("#transfer_button_id").button("disable");
    $('#open_wallet_button').on('click',  on_open_wallet);
    $('#transfer_button_id').on('click',  on_transfer);
    $('#generate_wallet_button').on('click',  on_generate_new_wallet);
    $('#close_wallet_button_id').on('click', on_close_wallet);
    $('#restore_wallet_button').on('click', on_restore_wallet);
    

    $('#sign_button_id').on('click', on_sign);

    $('#transfer_address_id').keyup(on_address_change);
    $('#transfer_address_id').on('change', on_address_change);

    $('#toggle-section-sign').click(function () {
      $(this).toggleClass('close');
      $('#section-sign').toggle();
    });

    $('#toggle-section-payment').click(function () {
      $(this).toggleClass('close');
      $('#section-payment').toggle();
    });

    $('#toggle-section-history').click(function () {
      $(this).toggleClass('close');
      $('#section-history').toggle();
    });

    setTimeout(init_btc_exchange_rate, 100);


    services['poloniex'].init('#plot_area');
    $('#exchange_view').on('visible');

    /****************************************************************************/
    //some testing stuff
    //to make it available in browser mode

    //test_parse_and_get_locktime_function();

    show_wallet();
    on_update_wallet_status({wallet_state: 2});
    var tttt = {
        ti:{
            height: 10,
            tx_hash: "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c212fe",
            payment_id: "17967407c304e12b659ce5ab696ad9f64005c809d3e214bc6834c5e1c1e9a9b5",
            amount: 10111100000000,
            tx_blob_size: 1222,
            is_income: true,
            unlock_time: 27,
            fee: 2000000000,
            timestamp: 1402878665,
            td: {
                rcv: [1000, 1000, 1000, 1000],//rcv: ["0.0000001000", "0.0000001000", "0.0000001000", "0.0000001000"],
                spn: [1000, 1000]//spn: ["0.0000001000", "0.0000001000"]
            },
            destinations: "1Htb4dS5vfR53S5RhQuHyz7hHaiKJGU3qfdG2fvz1pCRVf3jTJ12mia8SJsvCo1RSRZbHRC1rwNvJjkURreY7xAVUDtaumz",
            destination_alias: "just-mike"
        },
        balance: 1000,
        unlocked_balance: 1000
    };

    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tttt.ti, true));
    tttt.ti.tx_hash = "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c2152";
    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tttt.ti, true));
    tttt.ti.tx_hash = "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c2122";

    on_money_transfer(tttt);
    tttt.ti.tx_hash = "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    tttt.ti.timestamp = 1402171355;
    tttt.ti.fee = 1000000000;
    tttt.ti.payment_id = "";
    tttt.ti.amount =  10123000000000;
    tttt.ti.destinations = "1Htb4dS5vfR53S5RhQuHyz7hHaiKJGU3qfdG2fvz1pCRVf3jTJ12mia8SJsvCo1RSRZbHRC1rwNvJjkURreY7xAVUDtaumz";
    tttt.ti.destination_alias = "zoidberg";


    on_money_transfer(tttt);

    tttt.ti.tx_hash = "u19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    tttt.ti.payment_id = undefined;
    tttt.ti.destinations = "1Htb4dS5vfR53S5RhQuHyz7hHaiKJGU3qfdG2fvz1pCRVf3jTJ12mia8SJsvCo1RSRZbHRC1rwNvJjkURreY7xAVUDtaumz";
    tttt.ti.destination_alias = "tifozi";

    on_money_transfer(tttt);

    tttt.ti.tx_hash = "s19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = true;
    on_money_transfer(tttt);

    tttt.ti.tx_hash = "q19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    tttt.ti.height = 0;
    on_money_transfer(tttt);
    //
    tttt.ti.tx_hash = "8196q0a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = true;
    tttt.ti.height = 0;
    on_money_transfer(tttt);

    tttt.ti.tx_hash = "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c2152";
    $("#unconfirmed_transfers_container_id").prepend( get_transfer_html_entry(tttt.ti, false));
    /****************************************************************************/

    inline_menu_item_select(document.getElementById('daemon_state_view_menu'));

    //inline_menu_item_select(document.getElementById('wallet_view_menu'));

    Qt.update_daemon_state.connect(str_to_obj.bind({cb: on_update_daemon_state}));
    Qt.update_wallet_status.connect(str_to_obj.bind({cb: on_update_wallet_status}));
    Qt.update_wallet_info.connect(str_to_obj.bind({cb: on_update_wallet_info}));
    Qt.money_transfer.connect(str_to_obj.bind({cb: on_money_transfer}));

    Qt.show_wallet.connect(show_wallet);
    Qt.hide_wallet.connect(hide_wallet);
    Qt.switch_view.connect(on_switch_view);
    Qt.set_recent_transfers.connect(str_to_obj.bind({cb: on_set_recent_transfers}));
    Qt.handle_internal_callback.connect(on_handle_internal_callback);


    hide_wallet();
    on_update_wallet_status({wallet_state: 1});
    // put it here to disable wallet tab only in qt-mode
    disable_tab(document.getElementById('wallet_view_menu'), true);
    $("#version_text").text(Qt_parent.get_version());
    $("#version_text").text(Qt_parent.get_version());
});

function secure_request_url(url, params, callback_name) {
    if (window['Qt_parent'] && window['Qt_parent'] !== undefined) {
        return Qt_parent.request_uri(url, params, callback_name);
    }
    else
    {
        return window[callback_name](undefined);
    }
}

function on_polo_request_fetched(res)
{
    services['poloniex'].update(res);
}


function request_polo()
{
    if (!window['Qt_parent'] || window['Qt_parent'] === undefined) {
        on_polo_request_fetched($.parseJSON('[{"tradeID":"29295","date":"2014-08-30 23:50:10","type":"buy","rate":"0.00068","amount":"35.84051051","total":"0.02437155"},{"tradeID":"29294","date":"2014-08-30 23:48:40","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29293","date":"2014-08-30 23:47:57","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29292","date":"2014-08-30 23:47:16","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29291","date":"2014-08-30 23:47:16","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29290","date":"2014-08-30 23:47:05","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29289","date":"2014-08-30 23:46:55","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29288","date":"2014-08-30 23:46:49","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29287","date":"2014-08-30 23:46:33","type":"buy","rate":"0.00068","amount":"55.701","total":"0.03787668"},{"tradeID":"29286","date":"2014-08-30 23:46:33","type":"buy","rate":"0.000679","amount":"4.299","total":"0.00291902"},{"tradeID":"29285","date":"2014-08-30 23:46:25","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29284","date":"2014-08-30 23:46:01","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29283","date":"2014-08-30 23:45:51","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29282","date":"2014-08-30 23:45:38","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29281","date":"2014-08-30 23:45:15","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29280","date":"2014-08-30 23:45:10","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29279","date":"2014-08-30 23:44:52","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29278","date":"2014-08-30 23:44:37","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29277","date":"2014-08-30 23:44:28","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29276","date":"2014-08-30 23:44:06","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29275","date":"2014-08-30 23:43:46","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29274","date":"2014-08-30 23:43:43","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29273","date":"2014-08-30 23:43:19","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29272","date":"2014-08-30 23:43:05","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29271","date":"2014-08-30 23:42:56","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29270","date":"2014-08-30 23:42:43","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29269","date":"2014-08-30 23:42:24","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29268","date":"2014-08-30 23:42:22","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29267","date":"2014-08-30 23:42:12","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29266","date":"2014-08-30 23:42:03","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29265","date":"2014-08-30 23:42:01","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29264","date":"2014-08-30 23:41:53","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29263","date":"2014-08-30 23:41:41","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29262","date":"2014-08-30 23:41:30","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29261","date":"2014-08-30 23:41:19","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29260","date":"2014-08-30 23:41:09","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29259","date":"2014-08-30 23:41:02","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29258","date":"2014-08-30 23:40:59","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29257","date":"2014-08-30 23:40:49","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29256","date":"2014-08-30 23:40:38","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29255","date":"2014-08-30 23:40:28","type":"buy","rate":"0.00068","amount":"11.03","total":"0.0075004"},{"tradeID":"29254","date":"2014-08-30 23:40:26","type":"buy","rate":"0.00068","amount":"60","total":"0.0408"},{"tradeID":"29253","date":"2014-08-30 23:40:24","type":"buy","rate":"0.00067999","amount":"17.57397926","total":"0.01195013"},{"tradeID":"29252","date":"2014-08-30 23:40:18","type":"buy","rate":"0.00067999","amount":"11.03","total":"0.00750029"},{"tradeID":"29251","date":"2014-08-30 23:40:08","type":"buy","rate":"0.00067999","amount":"11.03","total":"0.00750029"},{"tradeID":"29250","date":"2014-08-30 23:39:57","type":"buy","rate":"0.00067999","amount":"11.03","total":"0.00750029"},{"tradeID":"29249","date":"2014-08-30 23:39:46","type":"buy","rate":"0.0006798","amount":"38.43226596","total":"0.02612625"},{"tradeID":"29248","date":"2014-08-30 23:37:39","type":"sell","rate":"0.0006798","amount":"53.56773404","total":"0.03641535"},{"tradeID":"29247","date":"2014-08-30 23:37:25","type":"buy","rate":"0.00068","amount":"876.45067647","total":"0.59598646"},{"tradeID":"29246","date":"2014-08-30 23:32:19","type":"sell","rate":"0.0006798","amount":"12.4","total":"0.00842952"},{"tradeID":"29245","date":"2014-08-30 23:31:03","type":"buy","rate":"0.00068","amount":"99.32267597","total":"0.06753942"},{"tradeID":"29244","date":"2014-08-30 23:31:03","type":"buy","rate":"0.00067983","amount":"47.73616227","total":"0.03245248"},{"tradeID":"29243","date":"2014-08-30 23:29:51","type":"buy","rate":"0.0006798","amount":"1.5968","total":"0.0010855"},{"tradeID":"29242","date":"2014-08-30 23:29:51","type":"buy","rate":"0.00067978","amount":"37.6907175","total":"0.0256214"},{"tradeID":"29241","date":"2014-08-30 23:20:48","type":"sell","rate":"0.00066602","amount":"1.6","total":"0.00106563"},{"tradeID":"29240","date":"2014-08-30 23:19:56","type":"sell","rate":"0.000666","amount":"258.45222035","total":"0.17212918"},{"tradeID":"29239","date":"2014-08-30 23:19:56","type":"sell","rate":"0.00066705","amount":"39.766","total":"0.02652591"},{"tradeID":"29238","date":"2014-08-30 23:19:56","type":"sell","rate":"0.00066706","amount":"50.76551028","total":"0.03386364"},{"tradeID":"29237","date":"2014-08-30 23:19:56","type":"sell","rate":"0.00066713","amount":"37.76625","total":"0.025195"},{"tradeID":"29236","date":"2014-08-30 23:17:29","type":"buy","rate":"0.0006759","amount":"20.958","total":"0.01416551"},{"tradeID":"29235","date":"2014-08-30 23:17:29","type":"buy","rate":"0.00067589","amount":"0.15237473","total":"0.00010299"},{"tradeID":"29234","date":"2014-08-30 23:17:29","type":"buy","rate":"0.000675","amount":"200","total":"0.135"},{"tradeID":"29233","date":"2014-08-30 23:17:29","type":"buy","rate":"0.00067195","amount":"372.05149193","total":"0.25"},{"tradeID":"29232","date":"2014-08-30 23:17:29","type":"buy","rate":"0.00067102","amount":"9.96429125","total":"0.00668624"},{"tradeID":"29231","date":"2014-08-30 23:17:29","type":"buy","rate":"0.0006701","amount":"880","total":"0.589688"},{"tradeID":"29230","date":"2014-08-30 23:17:29","type":"buy","rate":"0.00067002","amount":"898.83504164","total":"0.60223745"},{"tradeID":"29229","date":"2014-08-30 23:16:31","type":"buy","rate":"0.00067002","amount":"0.7176503","total":"0.00048084"},{"tradeID":"29228","date":"2014-08-30 23:16:09","type":"buy","rate":"0.00067002","amount":"47.83182592","total":"0.03204828"},{"tradeID":"29227","date":"2014-08-30 23:15:46","type":"buy","rate":"0.00067002","amount":"50.61548214","total":"0.03391339"},{"tradeID":"29226","date":"2014-08-30 23:15:46","type":"buy","rate":"0.00067","amount":"200","total":"0.134"},{"tradeID":"29225","date":"2014-08-30 23:15:45","type":"buy","rate":"0.000667","amount":"26.8873787","total":"0.01793388"},{"tradeID":"29224","date":"2014-08-30 23:15:45","type":"buy","rate":"0.000667","amount":"304.1","total":"0.2028347"},{"tradeID":"29223","date":"2014-08-30 23:15:45","type":"buy","rate":"0.000665","amount":"200","total":"0.133"},{"tradeID":"29222","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00066199","amount":"250","total":"0.1654975"},{"tradeID":"29221","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00066048","amount":"71.06093312","total":"0.04693433"},{"tradeID":"29220","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00066048","amount":"50","total":"0.033024"},{"tradeID":"29219","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00065999","amount":"136.19952888","total":"0.08989033"},{"tradeID":"29218","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00065999","amount":"64.299","total":"0.0424367"},{"tradeID":"29217","date":"2014-08-30 23:15:45","type":"buy","rate":"0.000655","amount":"200","total":"0.131"},{"tradeID":"29216","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00065499","amount":"0.16670552","total":"0.00010919"},{"tradeID":"29215","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00065","amount":"250","total":"0.1625"},{"tradeID":"29214","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00065","amount":"49.40384355","total":"0.0321125"},{"tradeID":"29213","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00064968","amount":"55.58660163","total":"0.0361135"},{"tradeID":"29212","date":"2014-08-30 23:15:45","type":"buy","rate":"0.0006488","amount":"14","total":"0.0090832"},{"tradeID":"29211","date":"2014-08-30 23:15:45","type":"buy","rate":"0.00063","amount":"500","total":"0.315"},{"tradeID":"29210","date":"2014-08-30 23:07:04","type":"buy","rate":"0.00062982","amount":"9.75","total":"0.00614074"},{"tradeID":"29209","date":"2014-08-30 23:07:04","type":"buy","rate":"0.00062959","amount":"60","total":"0.0377754"},{"tradeID":"29208","date":"2014-08-30 23:07:04","type":"buy","rate":"0.00062958","amount":"35.62699395","total":"0.02243004"},{"tradeID":"29207","date":"2014-08-30 23:07:04","type":"buy","rate":"0.00062957","amount":"38.43041499","total":"0.02419464"},{"tradeID":"29206","date":"2014-08-30 23:07:04","type":"buy","rate":"0.00062954","amount":"16.20603423","total":"0.01020235"},{"tradeID":"29205","date":"2014-08-30 23:07:04","type":"buy","rate":"0.00062951","amount":"58.23742003","total":"0.03666104"},{"tradeID":"29204","date":"2014-08-30 23:07:04","type":"buy","rate":"0.0006295","amount":"40.68204429","total":"0.02560935"},{"tradeID":"29203","date":"2014-08-30 22:42:23","type":"sell","rate":"0.000615","amount":"9.7843252","total":"0.00601736"},{"tradeID":"29202","date":"2014-08-30 22:40:37","type":"sell","rate":"0.00062999","amount":"6.59092776","total":"0.00415222"},{"tradeID":"29201","date":"2014-08-30 22:40:36","type":"buy","rate":"0.00062999","amount":"42.91192149","total":"0.02703408"},{"tradeID":"29200","date":"2014-08-30 22:38:30","type":"sell","rate":"0.00061501","amount":"38.50742985","total":"0.02368245"},{"tradeID":"29199","date":"2014-08-30 22:38:25","type":"sell","rate":"0.00061578","amount":"64.95826432","total":"0.04"},{"tradeID":"29198","date":"2014-08-30 22:37:46","type":"sell","rate":"0.00061579","amount":"42.99791732","total":"0.02647769"},{"tradeID":"29197","date":"2014-08-30 22:37:46","type":"sell","rate":"0.00061582","amount":"16.23851125","total":"0.01"},{"tradeID":"29196","date":"2014-08-30 22:37:46","type":"sell","rate":"0.00061584","amount":"40.76357143","total":"0.02510384"},{"tradeID":"29195","date":"2014-08-30 22:24:26","type":"sell","rate":"0.00064797","amount":"1.77649472","total":"0.00115112"},{"tradeID":"29194","date":"2014-08-30 22:24:25","type":"buy","rate":"0.00064797","amount":"407.82728849","total":"0.26425985"},{"tradeID":"29193","date":"2014-08-30 22:24:25","type":"buy","rate":"0.00064796","amount":"9.4906615","total":"0.00614957"},{"tradeID":"29192","date":"2014-08-30 22:24:25","type":"buy","rate":"0.00064796","amount":"11.57","total":"0.0074969"},{"tradeID":"29191","date":"2014-08-30 22:24:25","type":"buy","rate":"0.00064507","amount":"30","total":"0.0193521"},{"tradeID":"29190","date":"2014-08-30 22:24:25","type":"buy","rate":"0.00064455","amount":"5.3","total":"0.00341611"},{"tradeID":"29189","date":"2014-08-30 22:24:25","type":"buy","rate":"0.00064426","amount":"11.91496754","total":"0.00767634"},{"tradeID":"29188","date":"2014-08-30 22:24:25","type":"buy","rate":"0.00064425","amount":"5.00356698","total":"0.00322355"},{"tradeID":"29179","date":"2014-08-30 22:11:25","type":"sell","rate":"0.00061509","amount":"5.01359417","total":"0.00308381"},{"tradeID":"29178","date":"2014-08-30 22:01:31","type":"buy","rate":"0.000601","amount":"1.6019887","total":"0.0009628"},{"tradeID":"29177","date":"2014-08-30 22:01:10","type":"buy","rate":"0.00058","amount":"34.95833526","total":"0.02027583"},{"tradeID":"29176","date":"2014-08-30 22:01:10","type":"buy","rate":"0.00057999","amount":"101.55513179","total":"0.05890096"},{"tradeID":"29175","date":"2014-08-30 22:01:10","type":"buy","rate":"0.00057998","amount":"39.32729954","total":"0.02280905"},{"tradeID":"29174","date":"2014-08-30 21:59:22","type":"sell","rate":"0.00059","amount":"288","total":"0.16992"},{"tradeID":"29173","date":"2014-08-30 21:59:22","type":"sell","rate":"0.00059001","amount":"80.79173234","total":"0.04766793"},{"tradeID":"29172","date":"2014-08-30 21:55:27","type":"sell","rate":"0.00059014","amount":"1.6051991","total":"0.00094729"},{"tradeID":"29171","date":"2014-08-30 21:35:29","type":"buy","rate":"0.00064398","amount":"5","total":"0.0032199"},{"tradeID":"29170","date":"2014-08-30 21:35:09","type":"sell","rate":"0.00059001","amount":"20.72709917","total":"0.0122292"},{"tradeID":"29169","date":"2014-08-30 21:35:09","type":"sell","rate":"0.00059002","amount":"16.94886527","total":"0.01000017"},{"tradeID":"29168","date":"2014-08-30 21:34:40","type":"sell","rate":"0.00059001","amount":"0.41788371","total":"0.00024656"},{"tradeID":"29167","date":"2014-08-30 21:34:40","type":"sell","rate":"0.00059001","amount":"39.40611176","total":"0.02325"},{"tradeID":"29166","date":"2014-08-30 21:07:15","type":"sell","rate":"0.00064796","amount":"720.44887463","total":"0.46682205"},{"tradeID":"29165","date":"2014-08-30 21:07:07","type":"sell","rate":"0.00064796","amount":"11.57","total":"0.0074969"},{"tradeID":"29164","date":"2014-08-30 21:07:01","type":"buy","rate":"0.00064794","amount":"39.63369652","total":"0.02568026"},{"tradeID":"29163","date":"2014-08-30 20:53:52","type":"buy","rate":"0.00064797","amount":"42.47462074","total":"0.02752228"},{"tradeID":"29162","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00064797","amount":"1026.25257848","total":"0.66498088"},{"tradeID":"29161","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00064796","amount":"15.66859466","total":"0.01015262"},{"tradeID":"29160","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00064791","amount":"24.5766207","total":"0.01592344"},{"tradeID":"29159","date":"2014-08-30 20:52:41","type":"buy","rate":"0.000647","amount":"308.07250593","total":"0.19932291"},{"tradeID":"29158","date":"2014-08-30 20:52:41","type":"buy","rate":"0.000647","amount":"114.6","total":"0.0741462"},{"tradeID":"29157","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00064","amount":"4.72170613","total":"0.00302189"},{"tradeID":"29156","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00064","amount":"998","total":"0.63872"},{"tradeID":"29155","date":"2014-08-30 20:52:41","type":"buy","rate":"0.000639","amount":"11.53372632","total":"0.00737005"},{"tradeID":"29154","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00063898","amount":"302","total":"0.19297196"},{"tradeID":"29153","date":"2014-08-30 20:52:41","type":"buy","rate":"0.0006332","amount":"0.25015428","total":"0.0001584"},{"tradeID":"29152","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00063319","amount":"16.73233297","total":"0.01059475"},{"tradeID":"29151","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00060237","amount":"25.10018276","total":"0.0151196"},{"tradeID":"29150","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00060236","amount":"0.77192838","total":"0.00046498"},{"tradeID":"29149","date":"2014-08-30 20:52:41","type":"buy","rate":"0.0006015","amount":"10","total":"0.006015"},{"tradeID":"29148","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00060149","amount":"129.9852166","total":"0.07818481"},{"tradeID":"29147","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00058833","amount":"9.07256231","total":"0.00533766"},{"tradeID":"29146","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00058832","amount":"9.39541713","total":"0.00552751"},{"tradeID":"29145","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00057464","amount":"48.06629436","total":"0.02762082"},{"tradeID":"29144","date":"2014-08-30 20:52:41","type":"buy","rate":"0.00057454","amount":"0.897202","total":"0.00051548"},{"tradeID":"29143","date":"2014-08-30 20:41:41","type":"sell","rate":"0.00055001","amount":"0.899","total":"0.00049446"},{"tradeID":"29142","date":"2014-08-30 20:13:28","type":"sell","rate":"0.00051","amount":"242.03535704","total":"0.12343803"},{"tradeID":"29141","date":"2014-08-30 20:13:28","type":"sell","rate":"0.00051097","amount":"3.33610584","total":"0.00170465"},{"tradeID":"29140","date":"2014-08-30 20:13:28","type":"sell","rate":"0.000525","amount":"307","total":"0.161175"},{"tradeID":"29139","date":"2014-08-30 20:13:28","type":"sell","rate":"0.000525","amount":"250","total":"0.13125"},{"tradeID":"29138","date":"2014-08-30 20:13:28","type":"sell","rate":"0.0005255","amount":"132.37869692","total":"0.06956501"},{"tradeID":"29137","date":"2014-08-30 20:13:28","type":"sell","rate":"0.00052555","amount":"200","total":"0.10511"},{"tradeID":"29136","date":"2014-08-30 20:13:28","type":"sell","rate":"0.00052577","amount":"200","total":"0.105154"},{"tradeID":"29135","date":"2014-08-30 20:13:28","type":"sell","rate":"0.00052578","amount":"9.50968086","total":"0.005"},{"tradeID":"29134","date":"2014-08-30 20:13:28","type":"sell","rate":"0.00052578","amount":"80.57792544","total":"0.04236626"},{"tradeID":"29133","date":"2014-08-30 20:13:28","type":"sell","rate":"0.0005311","amount":"6.20209","total":"0.00329393"},{"tradeID":"29132","date":"2014-08-30 20:13:28","type":"sell","rate":"0.00053111","amount":"9.41424563","total":"0.005"},{"tradeID":"29131","date":"2014-08-30 20:12:12","type":"buy","rate":"0.00054999","amount":"5.02364145","total":"0.00276295"},{"tradeID":"29130","date":"2014-08-30 20:11:48","type":"sell","rate":"0.00055","amount":"250","total":"0.1375"},{"tradeID":"29129","date":"2014-08-30 20:11:48","type":"sell","rate":"0.00055001","amount":"9.0907438","total":"0.005"},{"tradeID":"29128","date":"2014-08-30 20:11:48","type":"sell","rate":"0.00055425","amount":"17.33947875","total":"0.00961041"},{"tradeID":"29127","date":"2014-08-30 20:11:48","type":"sell","rate":"0.000555","amount":"25","total":"0.013875"},{"tradeID":"29126","date":"2014-08-30 20:11:48","type":"sell","rate":"0.000556","amount":"75","total":"0.0417"},{"tradeID":"29125","date":"2014-08-30 20:11:48","type":"sell","rate":"0.00055656","amount":"61.63866895","total":"0.03430562"},{"tradeID":"29124","date":"2014-08-30 20:11:48","type":"sell","rate":"0.0005584","amount":"100","total":"0.05584"},{"tradeID":"29123","date":"2014-08-30 20:11:48","type":"sell","rate":"0.000559","amount":"10","total":"0.00559"},{"tradeID":"29122","date":"2014-08-30 20:11:48","type":"sell","rate":"0.000575","amount":"250","total":"0.14375"},{"tradeID":"29121","date":"2014-08-30 20:11:48","type":"sell","rate":"0.00057501","amount":"54.70149144","total":"0.0314539"},{"tradeID":"29120","date":"2014-08-30 20:11:48","type":"sell","rate":"0.0005751","amount":"1.60841593","total":"0.000925"},{"tradeID":"29119","date":"2014-08-30 20:11:48","type":"sell","rate":"0.00057512","amount":"43.22972427","total":"0.02486228"},{"tradeID":"29118","date":"2014-08-30 20:05:43","type":"buy","rate":"0.00059407","amount":"121.8873595","total":"0.07240962"},{"tradeID":"29117","date":"2014-08-30 20:05:42","type":"sell","rate":"0.00059012","amount":"17.96305711","total":"0.01060036"},{"tradeID":"29116","date":"2014-08-30 20:05:18","type":"buy","rate":"0.00059099","amount":"38.90136972","total":"0.02299032"},{"tradeID":"29115","date":"2014-08-30 20:04:29","type":"sell","rate":"0.000591","amount":"100","total":"0.0591"},{"tradeID":"29114","date":"2014-08-30 20:04:29","type":"sell","rate":"0.00059402","amount":"10.10117504","total":"0.0060003"},{"tradeID":"29113","date":"2014-08-30 19:55:29","type":"sell","rate":"0.00059608","amount":"93","total":"0.05543544"},{"tradeID":"29111","date":"2014-08-30 19:54:18","type":"sell","rate":"0.0005964","amount":"175.46348994","total":"0.10464643"},{"tradeID":"29110","date":"2014-08-30 19:54:06","type":"sell","rate":"0.0005964","amount":"124.53651006","total":"0.07427357"},{"tradeID":"29109","date":"2014-08-30 19:54:06","type":"sell","rate":"0.00059641","amount":"50.31300621","total":"0.03000718"},{"tradeID":"29108","date":"2014-08-30 19:54:06","type":"sell","rate":"0.00059641","amount":"25.15048373","total":"0.015"},{"tradeID":"29107","date":"2014-08-30 19:47:27","type":"sell","rate":"0.00059644","amount":"10.05513283","total":"0.00599728"},{"tradeID":"29106","date":"2014-08-30 19:47:09","type":"sell","rate":"0.00059646","amount":"16.7658647","total":"0.01000017"},{"tradeID":"29105","date":"2014-08-30 19:47:09","type":"sell","rate":"0.00059647","amount":"38.97932838","total":"0.02325"},{"tradeID":"28821","date":"2014-08-30 19:29:44","type":"buy","rate":"0.00059608","amount":"7","total":"0.00417256"},{"tradeID":"28820","date":"2014-08-30 19:26:26","type":"sell","rate":"0.00059607","amount":"11.55684","total":"0.00688869"},{"tradeID":"28819","date":"2014-08-30 19:24:36","type":"sell","rate":"0.00059608","amount":"1000","total":"0.59608"},{"tradeID":"28818","date":"2014-08-30 19:24:36","type":"sell","rate":"0.000597","amount":"300","total":"0.1791"},{"tradeID":"28817","date":"2014-08-30 19:24:36","type":"sell","rate":"0.00059701","amount":"52.65335589","total":"0.03143458"},{"tradeID":"28714","date":"2014-08-30 18:47:55","type":"sell","rate":"0.00059565","amount":"48.1626196","total":"0.02868806"},{"tradeID":"28713","date":"2014-08-30 18:47:55","type":"sell","rate":"0.00062002","amount":"1.8373804","total":"0.00113921"},{"tradeID":"28712","date":"2014-08-30 18:35:57","type":"sell","rate":"0.00062002","amount":"5","total":"0.0031001"},{"tradeID":"28711","date":"2014-08-30 18:33:58","type":"sell","rate":"0.00062002","amount":"10","total":"0.0062002"},{"tradeID":"28710","date":"2014-08-30 18:30:02","type":"sell","rate":"0.00062003","amount":"11.58","total":"0.00717995"},{"tradeID":"28708","date":"2014-08-30 18:24:50","type":"sell","rate":"0.00062","amount":"6.6898857","total":"0.00414773"},{"tradeID":"28707","date":"2014-08-30 18:24:50","type":"sell","rate":"0.00063","amount":"2","total":"0.00126"},{"tradeID":"28706","date":"2014-08-30 18:24:50","type":"sell","rate":"0.00063002","amount":"3.8101143","total":"0.00240045"},{"tradeID":"28705","date":"2014-08-30 18:24:37","type":"sell","rate":"0.00063002","amount":"12.475","total":"0.0078595"},{"tradeID":"28704","date":"2014-08-30 18:23:55","type":"sell","rate":"0.00063002","amount":"5.0898","total":"0.00320668"},{"tradeID":"28703","date":"2014-08-30 18:23:08","type":"sell","rate":"0.00063002","amount":"13.80923127","total":"0.00870009"},{"tradeID":"28701","date":"2014-08-30 18:20:42","type":"sell","rate":"0.00061502","amount":"12.5","total":"0.00768775"},{"tradeID":"28700","date":"2014-08-30 18:20:28","type":"sell","rate":"0.000615","amount":"12.5","total":"0.0076875"}]'));
        return;
    }


    req_arams = {
        method: "GET"
    };
    secure_request_url("https://poloniex.com/public?command=returnTradeHistory&currencyPair=BTC_BBR", JSON.stringify(req_arams), "on_polo_request_fetched");
}


var services = {};

function on_exchange_tab_switch(is_show)
{
    if(is_show)
    {
        window.setTimeout("services['poloniex'].render();", 100);
    }
}

services['poloniex'] = {

    js2plot: function(d) {
        var year, month, day, hour, min;
        year = String(d.getFullYear());
        month = String(d.getMonth() + 1);
        if (month.length == 1) {
            month = "0" + month;
        }
        day = String(d.getDate());
        if (day.length == 1) {
            day = "0" + day;
        }
        hour = String(d.getHours());
        if(hour.length == 1) {
            hour = "0" + hour;
        }
        min = String(d.getMinutes());
        if(min.length == 1) {
            min = "0" + min;
        }
        return month + "/" + day + "/" + year + " " + hour + ":" + min + ":00";
    },

    update: function(data) {
        if(data !== undefined)
        {
            console.log("fetched: https://poloniex.com/public?command=returnTradeHistory&currencyPair=BTC_BBR");
            $('#pololastprice').html(data[0].rate + "BTC");
            current_exchange_rate = parseFloat(data[0].rate);


            if(current_btc_to_usd_exchange_rate && current_btc_to_usd_exchange_rate !== undefined)
            {
                $('#exchange_rate_text').html(current_exchange_rate.toFixed(8) + "BTC/" + (current_exchange_rate*current_btc_to_usd_exchange_rate).toFixed(2) + "$" );
            }else
            {
                $('#exchange_rate_text').html(data[0].rate + "BTC");
            }


            console.log("Exchange rate BBR: " + current_exchange_rate.toFixed(8) + " BTC");

            if(services['poloniex'].lasttrade == undefined) {
                for(var i=data.length-1;i>=0;i--) {
                    $('#polochart').trigger('trade', data[i]);
                }
                services['poloniex'].lasttrade = data[0];
            } else {
                var i=data.length-1;
                while(i >= -1 && data[i].tradeID != services['poloniex'].lasttrade.tradeID) { i--; }
                if(i > 0) {
                    for(var ii=i-1; ii >= 0; ii--) {
                        $('#polochart').trigger('trade', data[ii]);
                    }
                    services['poloniex'].lasttrade = data[0];
                }
            }

            services['poloniex'].render();
        }else
        {
            $('#pololastprice').html('unable to receive market data');
        }
        window.setTimeout(request_polo, 1000000);
    },

    candles: [],
    ratemin: 10000000,
    ratemax: 0,
    lasttrade: undefined,
    plot: undefined,

    render: function() {
        console.log('render');
        if(services['poloniex'].plot != undefined) {
            services['poloniex'].plot.destroy();
        }
        services['poloniex'].plot = $.jqplot('polochart', [services['poloniex'].candles], {
            seriesColors: [ "#191A33" ],
            seriesDefaults: { yaxis: 'y2axis' },
            axes: {
                xaxis: {
                    renderer:$.jqplot.DateAxisRenderer,
                    tickOptions:{formatString:'<br/>%a<br/>%H:%M'},
                    min: services['poloniex'].candles[services['poloniex'].candles.length-1][0],
                    max: services['poloniex'].candles[0][0]
                },
                y2axis: {
                    tickOptions:{formatString:'&nbsp;&nbsp;%#.8fBTC'}
                }
            },
            series: [{renderer:$.jqplot.OHLCRenderer}],
            grid: {
                drawGridLines: true,
                gridLineColor: '#cccccc',
                background: '#F0F0F0',
                borderColor: '#191A33',
                borderWidth: 0,
                shadow: false
            },
            height:  300,
            width:  600,
            drawIfHidden: true
        });

    },

    init: function(jqtarget) {
        $(jqtarget).html('last price: <div id="pololastprice" style="display:inline;">loading...</div><div style="width: 600px;margin: 0 auto;"><div id="polochart" /></div>');
        $('#polochart').on('trade', function(ev, trade) {
            // { amount: "1.32222222", date: "2014-06-22 23:45:36", rate: "0.00232222", total: "0.00307049", tradeID: "13581", type: "sell"
            // 5min candles [O]pen [L]ow [H]igh [C]lose
            // [ ['06/15/2009 16:00:00', 136.01, 139.5, 134.53, 139.48]]
            var candleMins = 5;
            var t = trade.date.split(/[- :]/);
            var tradeDate = new Date(Date.UTC(t[0], t[1]-1, t[2], t[3], t[4], t[5], 0));
            var candleDate = new Date(Date.UTC(t[0], t[1]-1, t[2], t[3], parseInt(parseFloat(t[4]) / candleMins)*candleMins, 0, 0));
            var plotDate = services['poloniex'].js2plot(candleDate);
            var rate = parseFloat(trade.rate);

            if(rate > services['poloniex'].ratemax) {
                services['poloniex'].ratemax = rate;
            }

            if(rate < services['poloniex'].ratemin) {
                services['poloniex'].ratemin = rate;
            }

            if(services['poloniex'].candles.length == 0) {
                // first candle
                services['poloniex'].candles = [[plotDate, rate, rate, rate, rate]];
            } else if(services['poloniex'].candles[0][0] == plotDate) {
                // updating existing candle
                services['poloniex'].candles[0][4] = rate;
                if(rate > services['poloniex'].candles[0][3]) {
                    services['poloniex'].candles[0][3] = rate;
                }
                if(rate < services['poloniex'].candles[0][2]) {
                    services['poloniex'].candles[0][2] = rate;
                }
            } else {
                // new candle
                services['poloniex'].candles.unshift([plotDate, rate, rate, rate, rate]);
            }
        });
        request_polo();

    }
};



