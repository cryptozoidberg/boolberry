// Talks to C++ backend
Backend = function(emulator) {

    this.settings = {
//      useEmulator: true,
        intervalUpdateCurrentScreen: 5000
//      intervalCheckIfOnline: 5000
    };
    this.application = null;
    this.emulator = emulator;
    this.emulator.backend = this;
    this.last_daemon_state = null;
    var callbacks = [];
    var currentScreen = null;
    var $backend = this;

    this.safes = {};
    this.backendEventSubscribers = [];

    /******************** 'Talking to backend' stuff **********************/

    // Requests a function
    this.backendRequest = function(command, parameters, callback) {
        console.log("Requesting API command '"+command+"' with parameters: "+JSON.stringify(parameters));

        // Emulated call versus real one through the magic Qt object
        var commandFunction = (this.shouldUseEmulator()) ? this.emulator.backendRequestCall(command) : Qt[command];

        // Now call it
        var returnValue = commandFunction(parameters);
        var returnObject = JSON.parse(returnValue);

        if (returnObject.error_code != "OK") {
            console.log("API Error for command '"+command+"': " + returnObject.error_code);
        } else {
            // Everything is OK
            console.log("Requesting API command '"+command+"': completed, status=OK, request_id="+returnObject.request_id);
            callbacks[returnObject.request_id] = callback;
        }
    };

    // Callback from backend
    this.backendCallback = function(status, param) {
        // Do we have a callback for this?
        var requestId = status.request_id;
        if (callbacks[requestId]) {
            console.log("Request with id "+requestId+" successfully returned with status="+status.error_code+" and response:");
            console.log(param);
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

    this.subscribe = function(command, callback) {
        var nonBackendEvents = ['update_safe_count', 'update_balance'];

        if (nonBackendEvents.indexOf(command) >= 0) {
            // This object fires the event
            this.backendEventSubscribers[command] = callback;
        } else {
            // Backend layer fires the event

            if (this.shouldUseEmulator()) {
                this.emulator.eventCallbacks[command] = callback;
            } else {
                Qt[command].connect(callback);
            }
        }
    };

    this.fireEvent = function(event) {
        if (this.backendEventSubscribers[event]) {
            this.backendEventSubscribers[event]();
        }
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
         * update_daemon_state
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
        this.subscribe('update_daemon_state', function(data) {
            $backend.application.showOnlineState(data.text_state);

            // info widget
            $backend.last_daemon_state = data;
            $backend.application.updateBackendInfoWidget();
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
        this.subscribe('update_wallet_info', function(data) {
            // Save this array of safes if anything changed
            var id = data.wallet_id;
            var safe = $backend.safes[id];
            if (safe) {
                for(var key in data) {
                    if (data[key] != safe[key]) {
                        safe[key] = data[key];
                    }
                }
                $backend.application.reRenderSafe(id);
                $backend.fireEvent('update_balance');
            } else {
                // Got a new one? Just add it then
                $backend.safes[id] = data;

                // and notify the subscribers
                $backend.fireEvent('update_safe_count');
                $backend.fireEvent('update_balance');
            }
        });

    };

    this.showOpenFileDialog = function(caption, callback) {
        this.backendRequest('show_openfile_dialog', {caption: caption, filemask: '*.lui'}, callback);
    };

    this.closeWallet = function(wallet_id, callback) {
        // Prepare
        var callbackAwareOfWalletID = function(wallet_id) {
            return function(status, data) {
                // Remove this safe from internal storage
                if (status.error_code == 'OK') {
                    delete $backend.safes[ wallet_id ];

                    // Notify the subscribers
                    $backend.fireEvent('update_safe_count');
                    $backend.fireEvent('update_balance')
                }

                // Call the passed callback to reflect changes in UI
                callback(status, data);
            }
        };

        // Call API
        $backend.backendRequest('close_wallet', {wallet_id: wallet_id}, callbackAwareOfWalletID(wallet_id));
    };

    this.openWallet = function(file, name, pass, callback) {
        var param = {
            path: file,
            pass: pass
        };
        $backend.backendRequest('open_wallet', param, callback);
    };

    this.loadWalletToClientSide = function(wallet_id, callback) {
        $backend.backendRequest('get_wallet_info', {wallet_id: wallet_id}, function(status, data) {
            if (status.error_code == 'OK') {
                $backend.safes[wallet_id] = data;
                $backend.fireEvent('update_safe_count');
                $backend.fireEvent('update_balance');
            }

            callback(status, data);
        })
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

    this.shouldUseEmulator = function() {
        return (typeof Qt == 'undefined');
    };

}; // -- end of Backend definition

