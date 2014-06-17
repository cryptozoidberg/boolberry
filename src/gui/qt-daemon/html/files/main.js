
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
        $("#daemon_synchronization_text").text((info_obj.max_net_seen_height - info_obj.height).toString() + " blocks behind");
        $("#open_wallet_button").button("disable");
        $("#generate_wallet_button").button("disable");
        $("#domining_button").button("disable");
        //show progress
    }else if(info_obj.daemon_network_state == 2)//daemon_network_state_online
    {
        $("#synchronization_progressbar_block").hide();
        $("#daemon_status_text").addClass("daemon_view_general_status_value_success_text");
        $("#open_wallet_button").button("enable");
        $("#generate_wallet_button").button("enable");
        disable_tab(document.getElementById('wallet_view_menu'), false);
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

function get_details_block(td, div_id_str, transaction_id)
{
    var res = "<div class='transfer_entry_line_details' id='" + div_id_str + "'> <span class='balance_text'>Transaction ID:</span> " +  transaction_id + "<br>";
    if(td.rcv !== undefined)
    {
        for(var i=0; i < td.rcv.length; i++)
        {
            res += "<img src='files/recv.png' height='12px' /> " + td.rcv[i] + "<br>";
        }
    }
    if(td.spn !== undefined)
    {
        for(var i=0; i < td.spn.length; i++)
        {
            res += "<img src='files/sent.png' height='12px' /> " + td.spn[i] + "<br>";
        }
    }

    res += "</div>";
    return res;
}


function get_transfer_html_entry(tr)
{
    var res;
    var color_str;
    var img_ref;
    var action_text;

    if(tr.is_income)
    {
        color_str = "darkgreen";
        img_ref = "files/recv.png"
    }
    else
    {
        color_str = "darkmagenta";
        img_ref = "files/sent.png"
    }


    var dt = new Date(tr.timestamp*1000);

    res = "<div class='transfer_entry_line' style='color: " + color_str + "'>";
    res += "<img  src='" + img_ref + "' height='15px'>";
    res += "<span class='transfer_text_time_span'>" + dt.toLocaleTimeString() + "</span>";
    res += "<span class='transfer_text_amount_span'>" + tr.amount + "</span>";
    res += "<span class='transfer_text_details_span'>" + "<a href='javascript:;' onclick=\"jQuery('#" + tr.tx_hash + "_id" + "').toggle('fast');\" class=\"options_link_text\">" + "</span>";
    res += " Details...";
    res += "</a>";
    res += get_details_block(tr.td, tr.tx_hash + "_id", tr.tx_hash);
    res += "</div>";

    return res;
}

function on_update_wallet_info(wal_status)
{
    $("#wallet_balance").text(wal_status.balance);
    $("#wallet_unlocked_balance").text(wal_status.unlocked_balance);
    $("#wallet_address").text(wal_status.address);
    $("#wallet_path").text(wal_status.path);
}

function on_money_recieved(tei)
{
    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tei.ti));
    $("#wallet_balance").text(tei.balance);
    $("#wallet_unlocked_balance").text(tei.unlocked_balance);
}

function on_money_spent(tei)
{
    $("#recent_transfers_container_id").prepend( get_transfer_html_entry(tei.ti));
    $("#wallet_balance").text(tei.balance);
    $("#wallet_unlocked_balance").text(tei.unlocked_balance);
}

function on_open_wallet()
{
    Qt_parent.open_wallet();
}

function on_generate_new_wallet()
{
    Qt_parent.generate_wallet();
}

function show_wallet()
{
    //disable_tab(document.getElementById('wallet_view_menu'), false);
    //inline_menu_item_select(document.getElementById('wallet_view_menu'));
    $("#wallet_workspace_area").show();
    $("#wallet_welcome_screen_area").hide();
    $("#recent_transfers_container_id").html("");
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
function on_transfer()
{
    var transfer_obj = {
        destinations:[
            {
                address: $('#transfer_address_id').val(),
                amount: $('#transfer_amount_id').val()
            }
        ],
        mixin_count: 0,
        payment_id: $('#payment_id').val()
    };

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
        $('#mixin_count_id').val(0);
        $("#transfer_result_span").html("<span style='color: #1b9700'> Money successfully sent, transaction " + transfer_res_obj.tx_hash + ", " + transfer_res_obj.tx_blob_size + " bytes</span><br>");
        if(last_timerId !== undefined)
            clearTimeout(last_timerId);

        $("#transfer_result_zone").show("fast");
        last_timerId = setTimeout(function(){$("#transfer_result_zone").hide("fast");}, 15000);
    }
    else
        return;
}

function str_to_obj(str)
{
    var info_obj = jQuery.parseJSON(str);
    this.cb(info_obj);
}

$(function()
{ // DOM ready
    $( "#synchronization_progressbar" ).progressbar({value: false });
    $( "#wallet_progressbar" ).progressbar({value: false });
    $(".common_button").button();

    $("#open_wallet_button").button("disable");
    $("#generate_wallet_button").button("disable");
    $("#domining_button").button("disable");


    //$("#transfer_button_id").button("disable");
    $('#open_wallet_button').on('click',  on_open_wallet);
    $('#transfer_button_id').on('click',  on_transfer);
    $('#generate_wallet_button').on('click',  on_generate_new_wallet);
    $('#close_wallet_button_id').on('click',  on_close_wallet);

    /****************************************************************************/
    //some testing stuff
    //to make it available in browser mode
    show_wallet();
    on_update_wallet_status({wallet_state: 2});
    var tttt = {
        ti:{
            height: 10,
            tx_hash: "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c212fe",
            amount: "10.1111",
            is_income: true,
            timestamp: 1402878665,
            td: {
                rcv: ["0.0000001000", "0.0000001000", "0.0000001000", "0.0000001000"],
                spn: ["0.0000001000", "0.0000001000"]
            }
        },
        balance: "0.0000001000",
        unlocked_balance: "0.0000001000"
    };
    on_money_recieved(tttt);
    tttt.ti.tx_hash = "b19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    on_money_recieved(tttt);

    tttt.ti.tx_hash = "u19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    on_money_recieved(tttt);

    tttt.ti.tx_hash = "s19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = true;
    on_money_recieved(tttt);

    tttt.ti.tx_hash = "q19670a07875c0239df165ec43958fdbf4fc258caf7456415eafabc281c21c2";
    tttt.ti.is_income = false;
    on_money_recieved(tttt);
    /****************************************************************************/



    inline_menu_item_select(document.getElementById('daemon_state_view_menu'));

    //inline_menu_item_select(document.getElementById('wallet_view_menu'));

    Qt.update_daemon_state.connect(str_to_obj.bind({cb: on_update_daemon_state}));
    Qt.update_wallet_status.connect(str_to_obj.bind({cb: on_update_wallet_status}));
    Qt.update_wallet_info.connect(str_to_obj.bind({cb: on_update_wallet_info}));
    Qt.money_receive.connect(str_to_obj.bind({cb: on_money_recieved}));
    Qt.money_spent.connect(str_to_obj.bind({cb: on_money_spent}));
    Qt.show_wallet.connect(show_wallet);
    Qt.hide_wallet.connect(hide_wallet);
    Qt.switch_view.connect(on_switch_view);

    hide_wallet();
    on_update_wallet_status({wallet_state: 1});
    // put it here to disable wallet tab only in qt-mode
    disable_tab(document.getElementById('wallet_view_menu'), true);
    $("#version_text").text(Qt_parent.get_version());
});
