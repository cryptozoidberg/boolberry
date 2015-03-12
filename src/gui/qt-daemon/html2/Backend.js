// Talks to C++ backend
Backend = function(emulator) {

    this.settings = {
        useEmulator: true,
        intervalUpdateCurrentScreen: 5000
//      intervalCheckIfOnline: 5000
    };
    this.emulator = emulator;
    this.emulator.backend = this;
    var callbacks = [];
    var currentScreen = null;
    var $backend = this;

    this.safes = {};
    this.safesChangeCallbacks = [];

    /******************** 'Talking to backend' stuff **********************/

    // Requests a function
    this.backendRequest = function(command, parameters, callback) {
        // Emulated call versus real one through the magic Qt object
        var commandFunction = (this.settings.useEmulator) ? this.emulator.backendRequestCall(command) : Qt[command];

        // Now call it
        var returnValue = commandFunction(parameters);
        var returnObject = JSON.parse(returnValue);

        if (returnObject.error_code != "OK") {
            console.log("API Error: " + returnObject.error_code);
        } else {
            // Everything is OK
            callbacks[returnObject.request_id] = callback;
        }
    };

    // Callback from backend
    this.backendCallback = function(status, param) {
        // Everything is OK

        // Do we have a callback for this?
        var requestId = status.request_id;
        if (callbacks[requestId]) {
            var callback = callbacks[requestId];
            callback(status, param);
        } else {
            console.log("API Error: no such request_id: ["+requestId+"]");
        }
    };

    // Set global function shortcut for backend
    dispatch = function(status, param) {
        status = (status) ? JSON.parse(status) : null;
        param  = (param)  ? JSON.parse(param)  : null;
        return $backend.backendCallback(status, param);
    };

    /******************** Specific backend API calls being reflected in UI *********************/

    // UI initialization
    this.onAppInit = function() {
        this.registerEventCallbacks();

        // -- // -- // -- // -- //

        //setInterval(function() {
        //    $backend.checkIfOnline();
        //}, this.settings.intervalCheckIfOnline);
        //
        //$backend.checkIfOnline();
        //
        setInterval(this.updateCurrentScreen, this.settings.intervalUpdateCurrentScreen);

        $backend.emulator.startEventCallbacksTimers();
    };

    // Register callbacks for automatic events from backend side
    this.registerEventCallbacks = function() {
        /**
         * on_update_daemon_state
         *
         * data =  {
         *      "daemon_network_state": 2,
         *          "hashrate": 0,
         *          "height": 9729,
         *          "inc_connections_count": 0,
         *          "last_blocks": [
         *                           {
         *                               "date": 1425441268,
         *                               "diff": "107458354441446",
         *                               "h": 9728,
         *                               "type": "PoS"
         *                           },{
         *                               "date": 1425441256,
         *                               "diff": "2778612",
         *                               "h": 9727,
         *                               "type": "PoW"
         *                           }
         *                         ],
         *          "last_build_available": "0.0.0.0",
         *          "last_build_displaymode": 0,
         *          "max_net_seen_height": 9726,
         *          "out_connections_count": 2,
         *          "pos_difficulty": "107285151137540",
         *          "pow_difficulty": "2759454",
         *          "synchronization_start_height": 9725,
         *          "text_state": "Online"/"Offline"/"Loading"
         *  }
         *
         */
        this.subscribe('on_update_daemon_state', function(data) {
            $('.ifOnlineText').hide();
            $('.ifOnlineText.'+data.text_state.toLowerCase()).show();
        });

        /**
         *
         * data = {
         *          'wallets': [
         *              {
         *                  "wallet_id": "12345",
         *                  "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ",
         *                  "balance": 84555,
         *                  "do_mint": 1,
         *                  "mint_is_in_progress": 0,
         *                  "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_2.lui",
         *                  "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f",
         *                  "unlocked_balance": 84555
         *              }, // an array of those
         *          ]
         *        }
         *
         */
        this.subscribe('on_update_wallet_info', function(data) {
            // Save this array of safes if anything changed
            if (JSON.stringify($backend.safes) != JSON.stringify(data.wallets)) {

                $backend.safes = data.wallets;

                // Safe counter on index screen
                $('.index_safeCount').text($backend.safes.length);

                // If safes changed, show them on index screen

                // Destroy if first
                owlSafes.trigger('destroy.owl.carousel');
                owlSafes.html('');

                // Now add the items
                for (var i = 0; i < $backend.safes.length; i++) {
                    var safe = $backend.safes[i];
                    var safe_html = $('.safebox_template').html();
                    safe_html = safe_html.replace('{{ address }}', safe.address);
                    safe_html = safe_html.replace('{{ address }}', safe.address); // not a mistake
                    safe_html = safe_html.replace('{{ balance }}', safe.balance);
                    safe_html = safe_html.replace('{{ label }}', safe.wallet_id);
                    safe_html = safe_html.replace('{{ safe_id }}', safe.wallet_id);

                    owlSafes.append(safe_html);
                }

                // Calculate total balance
                var total_balance = 0;
                for (var i in $backend.safes) {
                    total_balance += $backend.safes[i].balance;
                }
                $('.index_totalBalance').text(total_balance);

                // And rebuild the carousel
                owlSafes.owlCarousel({
                    items: 2,
                    navText: '',
                    margin: 10
                });
            }
        });

    };

    this.subscribe = function(command, callback) {
        if (this.settings.useEmulator) {
            this.emulator.eventCallbacks[command] = callback;
        } else {
            Qt[command].connect(callback);
        }
    };

    this.showOpenFileDialog = function(caption, callback) {
        this.backendRequest('show_openfile_dialog', {caption: caption}, callback);
    };

    this.updateCurrentScreen = function() {
        if (currentScreen != null) $backend.updateScreen(currentScreen);
    };

    this.updateScreen = function(screen) {
        currentScreen = screen;
        switch(screen) {
            case 'index':
                //this.backendRequest('safeCount', null, function(status, data) {
                //    var count = (status.error_code == "OK") ? data.count : 'Error'; // todo: translate
                //    $('.index_safeCount').text(count);
                //});
                //this.backendRequest('paymentCount', null, function(status, data) {
                //    var count = (status.error_code == "OK") ? data.count : 'Error'; // todo: translate
                //    $('.index_paymentCount').text(count);
                //});
                //this.backendRequest('totalBalance', null, function(status, data) {
                //    var balance = (status.error_code == "OK") ? data.balance : 'Error'; // todo: translate
                //    $('.index_totalBalance').text(balance);
                //});
                //this.backendRequest('orderCount', null, function(status, data) {
                //    var count = (status.error_code == "OK") ? data.count : 'Error'; // todo: translate
                //    $('.index_orderCount').text(count);
                //});
                break;
        }
    };

}; // -- end of Backend definition

