// Backend emulation
Emulator = function() {

    this.backend = null; // set in Backend
    this.settings = {
        asyncEmulationTimeout: 500,
        eventsInterval: 5000
    };
    this.eventCallbacks = [];
    this.application = null;
    this.wallets = null;
    var lastRequestId = 0;
    var $emulator = this;

    // Emulation of saved app settings
    this.applicationSettings = {
        contacts: {
            10: {
                'name': 'Человек',
                'location': {
                    'city': 'Торонто',
                    'country': 'Канада'
                },
                'account': '1JYWzGRgYJuJhxSnsNSscrUe4xLwbnub4YJYWzGRgYJuJhxSnsNSscrUe4xLwbnub4Y1JYWzGRgYJuJhxSnsNSscrUe4xLwbnub4YJYWzGRgYJuJhxSnsNSscrUe4xLwbnub4Y',
                'aliases': [
                    'chelovek\'s_alias1',
                    'chelovek\'s_alias2'
                ],
                'contactFields': {
                    'Email': 'chelovek@domain.ru',
                    'Skype': 'chelovek',
                    'Facebook': 'chelovek3042'
                },
                'note': 'сказал, нужны статьи',
                'groups': [
                    'Проверенные'
                ]
            },
            11: {
                'name': 'Человек 2',
                'location': {
                    'city': 'Москва',
                    'country': 'Россия'
                },
                'account': '1JRgYWzGYJuJhJYWzxSnsNSscrUe4xLwbnub4YGRgYJuJhxSnsNSscrUe4xLwbnub4Y1JYWzGRgYJuJhxSnsNSscrUe4xLwbnub4YJYWzGRgYJuJhxSnsNSscrUe4xLwbnub4Y',
                'aliases': [
                    'chelovek2\'s_alias'
                ],
                'contactFields': {
                    'Email': 'chelovek2@mail.ru',
                    'Skype': 'chelovek2',
                    'Facebook': 'chelovechek'
                },
                'note': 'еще один контакт',
                'groups': [
                    'Проверенные', 'Магазины'
                ]
            }
        }
    };

    // Event callbacks (from backend)
    this.startEventCallbacksTimers = function() {
        setInterval(function() {
            var data;

            // update_daemon_state
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

            if ($emulator.eventCallbacks['update_daemon_state']) {
                $emulator.eventCallbacks['update_daemon_state'](data);
            }

        }, $emulator.settings.eventsInterval);

        // Init wallets with random data
        $emulator.wallets = {
            "12345": {
                "wallet_id": "12345",
                "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ",
                "balance": 84555,
                "do_mint": 1,
                "mint_is_in_progress": 0,
                "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_2.lui",
                "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f",
                "unlocked_balance": 84555
            },

            "12346": {
                "wallet_id": "12346",
                "address": "BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJHcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4",
                "balance": 55,
                "do_mint": 1,
                "mint_is_in_progress": 0,
                "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_4.lui",
                "tracking_hey": "6a193e5f6667c2291c8f361595ef1e9c3368d0fd4327fb64d896c013682bbad3",
                "unlocked_balance": 52
            },

            "12347": {
                "wallet_id": "12347",
                "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ",
                "balance": 4358746,
                "do_mint": 1,
                "mint_is_in_progress": 0,
                "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_2.lui",
                "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f",
                "unlocked_balance": 34234
            },

            "12348": {
                "wallet_id": "12348",
                "address": "BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJHcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4",
                "balance": 44,
                "do_mint": 1,
                "mint_is_in_progress": 0,
                "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_4.lui",
                "tracking_hey": "6a193e5f6667c2291c8f361595ef1e9c3368d0fd4327fb64d896c013682bbad3",
                "unlocked_balance": 33
            }
        };

        // show all the wallets in UI
        setTimeout(function() {
            for(var i in $emulator.wallets) {
                if ($emulator.eventCallbacks['update_wallet_info']) {
                    var wallet = $emulator.wallets[i];
                    $emulator.eventCallbacks['update_wallet_info']( JSON.parse(JSON.stringify(wallet)) );
                }
            }
        }, $emulator.settings.eventsInterval);

        // random update_wallet_info
        setInterval(function() {
            if (Object.size($emulator.wallets) > 0) {
                var rand_wallet = $emulator.application.getRandomObjectProperty($emulator.wallets);
                var rand_balance_shift = $emulator.application.getRandomInt(-30, 30);
                rand_wallet.balance += rand_balance_shift;
                if (rand_wallet.balance < 0) rand_wallet.balance = 1000;

                // Notify all the subscribers
                if ($emulator.eventCallbacks['update_wallet_info']) {
                    $emulator.eventCallbacks['update_wallet_info']( JSON.parse(JSON.stringify(rand_wallet)) );
                }
            }
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
                        var returnObject = $emulator.functions[arg.command].apply(this, JSON.parse(arg.arguments));

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
            var name = prompt('Тут будет диалог выбора файла: '+param.caption);
            return {error_code: "OK", data: {path: '/a/b/c/'+name}};
        },
        'close_wallet': function(param) {
            var id = param.wallet_id;
            if ($emulator.wallets[id]) {
                delete $emulator.wallets[id];
                return {error_code: "OK", data: {}};
            } else {
                return {error_code: "WRONG_WALLET_ID"};
            }
        },
        'open_wallet': function(param) {
            if (param.pass == '123') {
                var wallet_id = $emulator.application.getRandomInt(2000, 5000);
                $emulator.wallets[wallet_id] = {
                    "wallet_id": wallet_id,
                    "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ".shuffle(),
                    "balance": $emulator.application.getRandomInt(1000, 4000),
                    "do_mint": 1,
                    "mint_is_in_progress": 0,
                    "path": param.path,
                    "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f".shuffle(),
                    "unlocked_balance": $emulator.application.getRandomInt(0, 1000)
                };
                return {error_code: "OK", data: {wallet_id: wallet_id}};
            } else {
                return {error_code: "INVALID_PASSWORD"};
            }
        },
        'get_wallet_info': function(param) {
            var id = param.wallet_id;
            if ($emulator.wallets[id]) {
                return {error_code: "OK", data: $emulator.wallets[id]};
            } else {
                return {error_code: "WRONG_WALLET_ID"};
            }
        },
        'get_app_data': function(param) {
            return {error_code: "OK", data: $emulator.applicationSettings};
        },
        'store_app_data': function(param) {
            return {error_code: "OK", data: {}};
        }
    };

}; // -- end of Emulator

