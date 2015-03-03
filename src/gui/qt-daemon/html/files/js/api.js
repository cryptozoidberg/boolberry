/**
 * Created by roky on 03/03/15.
 */


/************* from native to javascript API (callbacks) ***************/

function on_update_daemon_state();
function on_update_wallet_status();
function on_update_wallet_info();
function on_money_transfer();

on_show_wallet();???
on_hide_wallet();???
on_switch_view_state();???
on_set_recent_transfers();???
update_pos_mining_text(); ???

/************* from javascript to native API ***************/

/* system api */
open_wallet();
generate_wallet();
close_wallet();
get_version();
QString transfer(const QString& json_transfer_object);
void message_box(const QString& msg);
QString request_uri(const QString& url_str, const QString& params, const QString& callbackname);
QString request_aliases();
bool init_config();
bool toggle_mining();
void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
QString get_exchange_last_top(const QString& params);
