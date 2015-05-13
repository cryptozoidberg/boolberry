(function() {
    'use strict';

    var module = angular.module('app.backendServices', [])

    module.factory('backend', ['$interval', '$timeout', 'emulator', 'loader', 'informer','$rootScope',
        function($interval, $timeout, emulator, loader, informer, $rootScope) {
        
        var callbacks = {};

        var loaders = {};

        var returnObject =  {
            
            // user functions

            openFileDialog : function(caption, filemask) {
                var params = {
                    caption: caption, 
                    filemask: filemask
                };

                return this.runCommand('show_openfile_dialog',params);
            },

            saveFileDialog : function(caption, filemask) {
                var params = {
                    caption: caption, 
                    filemask: filemask
                };

                return this.runCommand('show_savefile_dialog',params);
            },

            get_all_offers : function(callback) {
                var params = {};
                return this.runCommand('get_all_offers', params, callback);
            },

            resync_wallet : function(wallet_id) {
                var params = {
                    wallet_id: wallet_id
                }
                return this.runCommand('resync_wallet', params);
            },

            haveSecureAppData: function() { // for safes
                if(!this.shouldUseEmulator()){
                    var result = JSON.parse(Qt_parent['have_secure_app_data'](''));
                    return result.error_code === "TRUE" ? true : false;
                }else{
                    return true;
                }

            },

            registerAlias: function(wallet_id, alias, address, fee, comment, callback) {
                var params = {
                    "wallet_id": wallet_id,
                    "alias": {
                        "alias": alias,
                        "address": address,
                        "tracking_key": "",
                        "comment": comment
                    },
                    "fee": fee
                };

                return this.runCommand('request_alias_registration', params, callback);
            },

            getAllAliases: function(callback){
                return this.runCommand('get_all_aliases', {}, callback); 
            },

            getSecureAppData: function(pass) {
                if(!this.shouldUseEmulator()){
                    return Qt_parent['get_secure_app_data'](JSON.stringify(pass));
                }else{
                    return '{}';
                }
            },

            storeSecureAppData: function(data, pass) {
                console.log(pass);
                if(!this.shouldUseEmulator()){
                    return Qt_parent['store_secure_app_data'](JSON.stringify(data), pass);
                }else{
                    return '';
                }
            },

            getAppData: function() {
                if(!this.shouldUseEmulator()){
                    var data = Qt_parent['get_app_data']();
                    if(data){
                        return JSON.parse(data);
                    }else{
                        return false;
                    }
                    
                }else{
                    return false;
                }
            },

            storeAppData: function(data) {
                if(!this.shouldUseEmulator()){
                    return Qt_parent['store_app_data'](JSON.stringify(data));
                }else{
                    return '';
                }
            },

            pushOffer : function(wallet_id, offer_type, amount_lui, target, location, contacts, comment, expiration_time, fee, callback){
                // , amount_etc
                // , payment_types

                var params = {
                    "wallet_id" : wallet_id,
                    "od": {
                        "offer_type": offer_type, //0 buy, 1 sell
                        "amount_lui": amount_lui,
                        "amount_etc": 1,
                        "target": target,
                        "location": location,
                        "contacts": contacts,
                        "comment": comment,
                        "payment_types": "cash",
                        "expiration_time": expiration_time,
                        "fee" : fee
                    }
                };
                console.log(params);
                return this.runCommand('push_offer', params, callback);
            },

            transfer: function(from, to, ammount, fee, comment, push_payer, lock_time, is_mixin, callback) {
                 var params = {
                    wallet_id : from,
                    destinations : [
                        {
                            address : to,
                            amount : ammount
                        }
                    ],
                    mixin_count : is_mixin ? $rootScope.settings.security.mixin_count : 0,
                    lock_time : lock_time,
                    payment_id : "",
                    fee : fee,
                    comment: comment,
                    push_payer: push_payer 
                };
                console.log(params);
                return this.runCommand('transfer', params, callback);
            },

            quitRequest : function() {
                return this.runCommand('on_request_quit', {});
            },

            openWallet : function(file, pass, callback) {
                var params = {
                    path: file,
                    pass: pass
                };
                
                return this.runCommand('open_wallet', params, callback);
            },

            closeWallet : function(wallet_id, callback) {
                var params = {
                    wallet_id: wallet_id
                };
                
                return this.runCommand('close_wallet', params, callback);
            },

            getWalletInfo : function(wallet_id, callback) {
                var params = {
                    wallet_id: wallet_id
                };
                
                return this.runCommand('get_wallet_info', params, callback);
            },

            getRecentTransfers : function(wallet_id, callback) {
                var params = {
                    wallet_id: wallet_id
                };
                console.log('RUN get_recent_transfers, params = '+JSON.stringify(params));
                return this.runCommand('get_recent_transfers', params, callback);
            },

            generateWallet : function(path, pass, callback) {
                var params = {
                    path: path,
                    pass: pass
                };
                console.log('generate wallet params');
                console.log(params);
                return this.runCommand('generate_wallet', params, callback);
            },


            // system functions

            runCommand : function(command, params, callback) {
                var commandsNoLoading = [
                    'get_all_aliases'
                ];
                if(this.shouldUseEmulator()){
                    return emulator.runCommand(command, params, callback);
                }else{
                    var action = Qt_parent[command];
                    if(!angular.isDefined(action)){
                        console.log("API Error for command '"+command+"': command not found in Qt_parent object");
                    }else{
                        console.log(params);
                        var resultString = action(JSON.stringify(params)); 
                        var result = (resultString === '') ? {} : JSON.parse(resultString);
                        
                        console.log('API command ' + command +' call result: '+JSON.stringify(result));
                        
                        if(result.error_code == 'OK'){
                            if(angular.isDefined(callback)){
                                var request_id = result.request_id;
                                if(commandsNoLoading.indexOf(command) < 0){
                                    loaders[request_id] = loader.open();
                                }
                                callbacks[request_id] = callback;
                            }else{
                                return result; // If we didn't pass callback, its mean the function is synch
                            }
                        }else{
                            console.log("API Error for command '"+command+"': " + result.error_code);
                        }
                    }
                }
                
            },

            makeSend : function(tr) {
                var TS = 0;
                var mkTS = 0;
                if(tr.is_delay){
                    mkTS = tr.lock_time.getTime(); //miliseconds timestamp
                    TS = Math.floor(mkTS/1000); // seconds timestamp    
                }
                
                this.transfer(tr.from, tr.to, tr.ammount, tr.fee, tr.comment, tr.push_payer, TS, tr.is_mixin, function(data){
                    informer.success('Транзакция поступила в обработку');
                });
            },

            backendCallback: function (status, param) {
                console.log('DISPATCH: got result from backend');
                console.log(status);
                console.log(param);
                status = (status) ? JSON.parse(status) : null;
                param  = (param)  ? JSON.parse(param)  : null;
                
                var result = {};
                result.status = status;
                result.param = param;

                var request_id = status.request_id;
                var error_code = result.status.error_code;
                
                loaders[request_id].close();
                console.log('DISPATCH: got result from backend request id = '+request_id);
                console.log(result);

                var warningCodes = {
                    'FILE_RESTORED' : 'Мы определили что ваш файл кошелька был поврежден и восстановили ключи истории транзакций, синхронизация может занять какое-то время',
                    // some other warning codes
                };
                var is_warning = warningCodes.hasOwnProperty(error_code);

                if(error_code == 'OK' || error_code == '' || is_warning){
                    if(is_warning){
                        informer.warning(warningCodes[error_code]);
                    }
                    $timeout(function(){
                        callbacks[request_id](result.param); 
                    });
                    
                }else{
                    informer.error(result.status.error_code);
                    console.log(result.status.error_code);
                }
                
            },


            shouldUseEmulator : function() {
                var use_emulator = (typeof Qt == 'undefined');
                console.log("UseEmulator: " + use_emulator);
                return use_emulator;
            },

            callbackStrToObj : function(str) {
                var obj = $.parseJSON(str);
                this.callback(obj);
            },
            // need to link some repeating function from backend to our system
            subscribe : function(command, callback) {
                // var actualBackend = [
                //     'update_daemon_state', 'update_wallet_info'];

                // if (actualBackend.indexOf(command) >= 0) {
                    // *Deep backend layer* fires the event

                    if (this.shouldUseEmulator()) {
                        emulator.subscribe(command,callback);
                    } else {
                        console.log(command);
                        Qt[command].connect(this.callbackStrToObj.bind({callback: callback}));
                    }
                // } 
                // else {
                //     // *This service* fires the event
                //     console.log('Subscribe:: command is not supported '+command);
                //     this.backendEventSubscribers[command] = callback;
                // }
            },

        }

        if (!returnObject.shouldUseEmulator()) {
            console.log('Subscribed on "dispatch"');
            Qt["do_dispatch"].connect(returnObject.backendCallback); // register backend callback
        }
        

        return returnObject;

    }]);

    module.factory('emulator', ['$interval', '$timeout', 'loader','informer',
        function($interval, $timeout, loader, informer){
        var callbacks = {};

        this.getAMData = function() { //generate one year of mining data
            var data = [];
            var avarageValue = Math.random()*100;
            var startDate = new Date() - 1000*60*60*24*365; //one year before
            var nextDate = startDate;
            var step = 0;
            for(var i=1; i!=365; i++){
                step = Math.random()*10 - 5;
                data.push([nextDate, avarageValue+step]);
                nextDate += 1000*60*60*24;
            }
            return data;
        };
        this.getWalletInfo = function() {
            
            var random_balance = function(){
                return Math.floor(Math.random() * 100) * 10000000000;
            }

            var object = {
                "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ",
                "balance": random_balance(),
                "do_mint": 1,
                "mint_is_in_progress": 0,
                "path": "\/Users\/roky\/projects\/louidor\/wallets\/mac\/roky_wallet_small_2.lui",
                "tracking_hey": "d4327fb64d896c013682bbad36a193e5f6667c2291c8f361595ef1e9c3368d0f",
                "unlocked_balance": random_balance()
            }
            console.log(object);
            return object;
        };
        this.runCommand = function(command, params, callback) {
            var commandsNoLoading = [
                'get_all_aliases'
            ];
            var isShowLoader = commandsNoLoading.indexOf(command) < 0;
            var data = {};
            var commandData = false;
            if(commandData = this.getData(command)){
                data = commandData;
            }
            if(angular.isDefined(callback)){
                
                if(isShowLoader){
                    var loaderInst = loader.open('Run command ' + command + ' params = ' + JSON.stringify(params));
                }

                $timeout(function(){
                    if(isShowLoader) loaderInst.close();
                    callback(data);
                },3000);
                
            }else{
                console.log('fire event '+command);
                return data;
            }
            
        };
        this.subscribe = function(command, callback) {
            var data = {};
            var commandData = false;
            if(commandData = this.getData(command)){
                data = commandData;
            }

            var interval = $interval(function(){
                //data.daemon_network_state = Math.floor(Math.random()*3);
                callback(data);
            },3000);
        };
        this.getData = function(command){
            var result = false;

            switch (command){
                case 'show_openfile_dialog' : 
                    result = { 
                        "error_code": "OK",
                        "path": "/home/master/Lui/test_wallet.lui"
                    };
                    break;
                case 'show_savefile_dialog' : 
                    result = { 
                        "error_code": "OK",
                        "path": "/home/master/Lui/test_wallet_for_save.lui"
                    };
                    break;
                case 'open_wallet' : 
                    result = {
                        "wallet_id" : "1"
                    }
                    break;
                case 'generate_wallet' : 
                    result = {
                        param: {"wallet_id" : "1"}
                    }
                    break;
                case 'get_all_offers':
                    result = {
                      "offers": [
                        {
                            "offer_type": 0,
                            "amount_lui": 2300000000,
                            "amount_etc": 2,
                            "target": "EUR",
                            "location": "USA, NYC",
                            "contacts": "+89876782342",
                            "comment": "Best ever service",
                            "payment_types": "cash",
                            "expiration_time":3
                        }, 
                        {
                            "offer_type": 0,
                            "amount_lui": 4300000000,
                            "amount_etc": 2,
                            "target": "EUR",
                            "location": "USA, Washington",
                            "contacts": "+89876782342",
                            "comment": "Best ever service",
                            "payment_types": "cash",
                            "expiration_time":5
                        }
                      ]
                    };
                    break;
                case 'get_recent_transfers' :
                    result = {
                        'history': [
                            {
                                '$$hashKey': "object:45",
                                'amount': 1000000000000,
                                'fee': 1000000000000,
                                'height': 2616,
                                'is_income': false,
                                'payment_id': "",
                                'remote_address': "HhTZP7Sy4FoDR1kJHbFjzd5gSnUPdpHWHj7Gaaeqjt52KS23rHGa1sN73yZYPt77TkN8VVmHrT5qmBJQGzDLYJGjQpxGRid",
                                'recipient_alias': "",
                                'td': {
                                    'rcv': [
                                        9000000000000,
                                        10000000000000,
                                        80000000000000,
                                        100000000000000,
                                    ],
                                    'spn': [200000000000000]
                                },
                                'timestamp': 1429715920,
                                'tx_blob_size': 311,
                                'tx_hash': "62b4f42bcff74c05199a4961ed226b542db38ea2bffbb4c2c384ee3b84f34e59",
                                'unlock_time': 0
                            },
                            {
                                '$$hashKey': "object:46",
                                'amount': 200000000000000,
                                'fee': 1000000000,
                                'height': 1734,
                                'is_income': true,
                                'payment_id': "",
                                'remote_address': "",
                                'recipient_alias': "",
                                'td': {
                                    'rcv': [200000000000000]
                                },
                                'length': 1,
                                'timestamp': 1429608249,
                                'tx_blob_size': 669,
                                'tx_hash': "514fa3ba101df74bb4ce2c8f8653cd5a9d7c9d5777a4a587878bb5b6cd5954b9",
                                'unlock_time': 0
                            }

                        ]

                    }
                    break;
                case 'get_wallet_info' : 
                    result = this.getWalletInfo();
                    break;
                case 'update_wallet_info' :
                    result = {
                        wallets  : [
                            {
                                wallet_id: "1",
                                wi : this.getWalletInfo()
                            }
                        ]
                    };
                    break;
                case 'update_daemon_state': 
                    result = {
                        "daemon_network_state": 2,
                        "hashrate": 0,
                        "max_net_seen_height": 9800,
                        "synchronization_start_height": 9700,
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

                        "out_connections_count": 2,
                        "pos_difficulty": "107285151137540",
                        "pow_difficulty": "2759454",
                        
                        "text_state": "Offline"
                    };
                    break;
                default:
                    result = false;
            }

            return result;
        }
        return this;
    }]);

    

}).call(this);