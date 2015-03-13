// Backend emulation
Emulator = function() {

    this.backend = null; // set in Backend
    this.settings = {
        asyncEmulationTimeout: 500,
        eventsInterval: 1000
    };
    this.eventCallbacks = [];
    this.wallets = null;
    var lastRequestId = 0;
    var $emulator = this;

    // Event callbacks (from backend)
    this.startEventCallbacksTimers = function() {
        setInterval(function() {
            var data;

            // on_update_daemon_state
            data = {
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
                    }, {
                        "date": 1425441256,
                        "diff": "2778612",
                        "h": 9727,
                        "type": "PoW"
                    }
                ],
                "last_build_available": "0.0.0.0",
                "last_build_displaymode": 0,
                "max_net_seen_height": 9726,
                "out_connections_count": 2,
                "pos_difficulty": "107285151137540",
                "pow_difficulty": "2759454",
                "synchronization_start_height": 9725,
                "text_state": "Offline"
            };
            $emulator.eventCallbacks['on_update_daemon_state'](data);

        }, $emulator.settings.eventsInterval);

        // on_update_wallet_info
        setInterval(function() {
            var data;

            // on_update_daemon_state
            if ($emulator.wallets == null) {
                $emulator.wallets = [
                        {
                            "wallet_id": "12345",
                            "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ",
                            "balance": 84555,
                            "do_mint": 1,
                            "mint_is_in_progress": 0,
                            "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_2.lui",
                            "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f",
                            "unlocked_balance": 84555
                        },

                        {
                            "wallet_id": "12346",
                            "address": "BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJHcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4",
                            "balance": 55,
                            "do_mint": 1,
                            "mint_is_in_progress": 0,
                            "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_4.lui",
                            "tracking_hey": "6a193e5f6667c2291c8f361595ef1e9c3368d0fd4327fb64d896c013682bbad3",
                            "unlocked_balance": 52
                        },

                        {
                            "wallet_id": "12347",
                            "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ",
                            "balance": 4358746,
                            "do_mint": 1,
                            "mint_is_in_progress": 0,
                            "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_2.lui",
                            "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f",
                            "unlocked_balance": 34234
                        },

                        {
                            "wallet_id": "12348",
                            "address": "BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJHcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4",
                            "balance": 44,
                            "do_mint": 1,
                            "mint_is_in_progress": 0,
                            "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_4.lui",
                            "tracking_hey": "6a193e5f6667c2291c8f361595ef1e9c3368d0fd4327fb64d896c013682bbad3",
                            "unlocked_balance": 33
                        }
                ];
            }
            $emulator.eventCallbacks['on_update_wallet_info']( {wallets: JSON.parse(JSON.stringify($emulator.wallets)) } );

        }, $emulator.settings.eventsInterval);

        //create-safe_safe-name-input
        //create-safe_safe-password-input-1
        //create-safe_safe-password-input-2
    };

    // Should return a function to be called with the arguments later
    this.backendRequestCall = function(command) {
        if (command in this.functions) {
            return function(/*arguments*/) {
                // Make up request ID
                var requestId = ++lastRequestId;

                // Emulate async call using timeout
                setTimeout(function(arg)
                    {
                        // Actually calling the command with arguments of parent function
                        var returnObject = $emulator.functions[arg.command].apply(this, arg.arguments);

                        // Passing the emulated result to our UI callback
                        var status = {
                            error_code: returnObject.error_code,
                            request_id: arg.requestId
                        };
                        $emulator.sendAnswer(status, returnObject.data);
                    },
                    $emulator.settings.asyncEmulationTimeout,
                    {command: command, requestId: requestId, arguments: arguments}
                );

                // Successful API call
                return JSON.stringify({
                    error_code: "OK",
                    request_id: requestId
                });
            }
        } else {
            // There's no such command
            return function(/*arguments*/) {
                console.log("Emulator error: WRONG_API_COMMAND: "+command);
                return JSON.stringify({
                    error_code: "WRONG_API_COMMAND"
                });
            };
        }
    };

    this.sendAnswer = function(status, data) {
        dispatch(JSON.stringify(status), JSON.stringify(data));
    };

    this.functions = {
        'show_openfile_dialog': function(param) {
            var name = prompt('Имя файла для открытия: '+param.caption);
            return {error_code: "OK", data: {path: '/a/b/c/'+name}};
        },
        'close_wallet': function(param) {
            var id = param.wallet_id;
            for(var i in $emulator.wallets) {
                if ($emulator.wallets[i].wallet_id == id) {
                    $emulator.wallets.splice(i, 1);
                    return {error_code: "OK", data: {}};
                }
            }
            return {error_code: "WRONG_WALLET_ID"};
        }
    };

}; // -- end of Emulator

