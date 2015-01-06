/*
 sample result:
                    <tr data-trxid="trx_id">
                        <td class="iconcell" data-type="unconfirmed">&nbsp;</td>
                        <td>
                            <div class="infocellheader">AMOUNT</div>
                            <div class="infocell">100.000000000000</div>
                        </td>
                        <td>
                            <div class="infocellheader">STATUS</div>
                            <div class="infocell">UNCONFIRMED</div>
                        </td>
                        <td>
                            <div class="infocellheader">DATE</div>
                            <div class="infocell">2014-10-22 22:58</div>
                        </td>
                        <td>
                            <div class="infocellheader">SENDER</div>
                            <div class="infocell">N/A</div>
                        </td>
                        <td class="infocellmoreicon">
                            <div class="infocellheader">MORE</div>
                            <div class="infocell">DETAILS...</div>
                        </td>
                    </tr>
 
*/

function get_trx_icon_cell(data, confirmed) {
    console.log("get_trx_icon_cell");
    var res = '<td class="iconcell" data-type="';
    if (confirmed) {
        if (data.is_income)
            res += 'in';
        else
            res += 'out';
    } else {
        res += 'unconfirmed';
    }
    res += '">&nbsp;</td>';
    return res;
}

function get_trx_amount_cell(data, confirmed) {
    console.log("get_trx_amount_cell");
    var res = '<td><div class="infocellheader">AMOUNT</div><div class="infocell">';
    res += print_money(data.amount);
    res += '</div></td>';
    return res;
}

function get_trx_status_cell(data, confirmed) {
    console.log("get_trx_status_cell");
    var res = '<td><div class="infocellheader">STATUS</div><div class="infocell">';
    if (confirmed) {
        if (data.is_income)
            res += 'RECEIVED';
        else
            res += 'SENT';
    } else {
        res += 'UNCONFIRMED';
    }
    res += '</div></td>';
    return res;
}

function get_trx_date_cell(data, confirmed) {
    console.log("get_trx_date_cell");
    var res = '<td><div class="infocellheader">DATE</div><div class="infocell">';
    var dt = new Date(data.timestamp * 1000);   
    res += dt.toDateString(); //dt.format("yyyy-mm-dd HH:MM");
    res += '</div></td>';
    return res;
}

function get_trx_sender_receiver_cell(data, confirmed) {
    console.log("get_trx_sender_receiver_cell");
    var res = '<td><div class="infocellheader">';
    if (data.is_income)
        res += 'SENDER';
    else
        res += 'RECEIVER';
    res += '</div><div class="infocell">';
    if (data.is_income) {
        res += 'N/A';
    } else {
        if (data.recipient_alias)
            res += data.recipient_alias;
        else
            res += '...';
    }
    res += '</div></td>';
    return res;
}

function get_trx_details_cell(data, confirmed) {
    console.log("get_trx_details_cell");
    var res =
        '<td>' +
        '<div class="infocellmoreicon">' +
            '<div class="infocellheader">MORE</div>' +
            '<div class="infocell">DETAILS...</div>' +
        '</div>' +
        '<div class="trxdetails">' +
            'Amount: ' + print_money(data.amount) + '<br/>' +
            'Fee: ' + print_money(data.fee) + '<br/>' +
            'Height: ' + data.height + '<br/>';
    if (data.is_income)
        res += 'Type: in<br/>';
    else
        res += 'Type: out<br/>';
    res +=
            'Payment id: ' + data.payment_id + '<br/>' +
            'Recipient: ' + data.recipient + '<br/>' +
            'Recipient alias: ' + data.recipient_alias + '<br/>' +
            'Details: ' + JSON.stringify(data.td) + '<br/>' +
            'Timestamp: ' + data.timestamp + '<br/>' +
            'Size: ' + data.tx_blob_size + '<br/>' +
            'Hash: ' + data.tx_hash + '<br/>' +
            'Unlock time: ' + data.unlock_time +
        '</div>' +
        '</td>';
    console.log(res);
    return res;
}

function get_trx_table_entry(data, confirmed) {
    console.log("get_trx_table_entry(" + JSON.stringify(data) + ", " + confirmed + ")");
    return "<tr data-trxid='" + data.tx_hash + "'>" + get_trx_icon_cell(data, confirmed) +
        get_trx_amount_cell(data, confirmed) + 
        get_trx_status_cell(data, confirmed) + 
        get_trx_date_cell(data, confirmed) + 
        get_trx_sender_receiver_cell(data, confirmed) + 
        get_trx_details_cell(data, confirmed) + 
        "</tr>";
}
