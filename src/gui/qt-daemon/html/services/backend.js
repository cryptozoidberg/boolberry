(function() {
    'use strict';

    var module = angular.module('app.backendServices', [])

    module.factory('backend', ['$interval', '$timeout', 'emulator', 'loader', 'informer','$rootScope', '$filter',
        function($interval, $timeout, emulator, loader, informer, $rootScope, $filter) {
        
        var callbacks = {};

        var loaders = {};

        var returnObject =  {
            
            // user functions

            openFileDialog : function(caption, filemask) {
                var dir = '/';
                if(angular.isDefined($rootScope.settings.system.default_user_path)){
                    dir = $rootScope.settings.system.default_user_path;
                }
                var params = {
                    caption: caption, 
                    filemask: filemask,
                    default_dir: dir
                };
                return this.runCommand('show_openfile_dialog',params);
            },

            webkitLaunchedScript : function(){
                this.runCommand('webkit_launched_script',{});
            },

            toggleAutostart: function(value) {
                var params = {
                    v: value
                };
                this.runCommand('toggle_autostart',params);
            },

            isAutostartEnabled: function() {
                if(!this.shouldUseEmulator()){
                    var res = Qt_parent['is_autostart_enabled']();
                }else{
                    var res = {
                      "error_code": true
                    };
                }
                return res.error_code;
            },

            saveFileDialog : function(caption, filemask) {
                var dir = '/';
                if(angular.isDefined($rootScope.settings.system.default_user_path)){
                    dir = $rootScope.settings.system.default_user_path;
                }
                var params = {
                    caption: caption, 
                    filemask: filemask,
                    default_dir: dir
                };

                return this.runCommand('show_savefile_dialog',params);
            },

            setLogLevel : function(level){
                var params = {
                    v: level
                };
                this.runCommand('set_log_level',params);
            },

            get_all_offers : function(callback) {
                var params = {};
                return this.runCommand('get_all_offers', params, callback);
            },

            resetWalletPass: function(wallet_id, pass) {
                var params = {
                    wallet_id: wallet_id,
                    pass: pass
                };
                return this.runCommand('reset_wallet_password', params);
            },

            getVersion : function() {
                if(!this.shouldUseEmulator()){
                    var res = Qt_parent['get_version']();
                }else{
                    var res = '0.0.0.0';
                }
                return res;
            },

            getSmartSafeInfo : function(wallet_id) {
                var params = {
                    wallet_id : wallet_id
                };
                return this.runCommand('get_smart_safe_info', params);
            },

            restoreWallet : function(path, pass, restore_key, callback) {
                var params = {
                    restore_key : restore_key,
                    path: path,
                    pass: pass
                };
                return this.runCommand('restore_wallet', params, callback);
            },

            startPosMining : function(wallet_id) {
                var params = {
                    wallet_id : wallet_id
                };
                // if($rootScope.daemon_network_state.pos_allowed){
                    return this.runCommand('start_pos_mining', params);
                // }
            },

            stopPosMining : function(wallet_id) {
                var params = {
                    wallet_id : wallet_id
                };
                return this.runCommand('stop_pos_mining', params);
            },

            resync_wallet : function(wallet_id, callback) {
                var params = {
                    wallet_id: wallet_id
                }
                return this.runCommand('resync_wallet', params, callback);
            },

            haveSecureAppData: function() { // for safes
                if(!this.shouldUseEmulator()){
                    var result = JSON.parse(Qt_parent['have_secure_app_data'](''));
                    return result.error_code === "TRUE" ? true : false;
                }else{
                    return true;
                }

            },

            registerAlias: function(wallet_id, alias, address, fee, comment, reward, callback) {
                var params = {
                    "wallet_id": wallet_id,
                    "alias": {
                        "alias": alias,
                        "address": address,
                        "tracking_key": "",
                        "comment": comment,
                    },
                    "fee": $filter('gulden_to_int')(fee),
                    "reward": $filter('gulden_to_int')(reward)
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
                if(!this.shouldUseEmulator()){
                    return Qt_parent['store_secure_app_data'](JSON.stringify(data), pass);
                }else{
                    return '';
                }
            },

            storeFile: function(path, buff) {
                if(!this.shouldUseEmulator()){
                    return Qt_parent['store_to_file'](path, JSON.stringify(buff));
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

            getMiningEstimate: function(amount_coins, time) {

                var params = {
                  "amount_coins": $filter('gulden_to_int')(amount_coins),
                  "time": parseInt(time)  //in seconds, 43200000 sec = 500 days
                };
                // informer.info(JSON.stringify(params));
                return this.runCommand('get_mining_estimate', params);
            },

            backupWalletKeys: function(wallet_id, path) {

                var params = {
                  "wallet_id": wallet_id,
                  "path": path
                };

                return this.runCommand('backup_wallet_keys', params);
            },

            pushOffer : function(
                wallet_id, offer_type, amount_lui, target, location_city, location_country, contacts, 
                comment, expiration_time, fee, amount_etc, payment_types, bonus, deal_option, callback){
                var params = {
                    "wallet_id" : wallet_id,
                    "od": {
                        "offer_type": offer_type, //0 buy, 1 sell
                        "amount_lui": $filter('gulden_to_int')(amount_lui),
                        "amount_etc": parseInt(amount_etc),//$filter('gulden_to_int')(amount_etc),
                        "target": target,
                        "location_city":    location_city,
                        "location_country": location_country,
                        "contacts": contacts,
                        "comment": comment,
                        "payment_types": payment_types,
                        "expiration_time": expiration_time,
                        "fee" : $filter('gulden_to_int')(fee),
                        "bonus" : bonus,
                        "deal_option" : deal_option
                    }
                };
                //if(angular.isDefined(bonus) && bonus) params.od.bonus = parseInt(bonus);
                return this.runCommand('push_offer', params, callback);
            },

            pushUpdateOffer : function(
                wallet_id, tx_hash, offer_type, amount_lui, target, location_city, location_country, contacts, 
                comment, expiration_time, fee, amount_etc, payment_types, bonus, deal_option, callback){
                var params = {
                    "wallet_id" : wallet_id,
                    "tx_hash" : tx_hash,
                    "no" : 0,
                    "od": {
                        "offer_type": offer_type, //0 buy, 1 sell
                        "amount_lui": $filter('gulden_to_int')(amount_lui),
                        "amount_etc": parseInt(amount_etc),//$filter('gulden_to_int')(amount_etc),
                        "target": target,
                        "location_city":    location_city,
                        "location_country": location_country,
                        "contacts": contacts,
                        "comment": comment,
                        "payment_types": payment_types,
                        "expiration_time": expiration_time,
                        "fee" : $filter('gulden_to_int')(fee),
                        "bonus" : bonus,
                        "deal_option" : deal_option
                        
                    }
                };
                //if(angular.isDefined(bonus) && bonus) params.od.bonus = parseInt(bonus);
                return this.runCommand('push_update_offer', params, callback);
            },

            getAliasCoast: function(alias){
                var params = {
                    v: alias
                };
                return this.runCommand('get_alias_coast', params);
            },

            cancelOffer: function(wallet_id,tx_hash,callback){
                var params = {
                    wallet_id: wallet_id,
                    tx_hash: tx_hash,
                    no : 0 // ???
                };
                return this.runCommand('cancel_offer', params, callback);
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
                    fee : $filter('gulden_to_int')(fee),
                    comment: comment,
                    push_payer: push_payer 
                };
                console.log(params);
                return this.runCommand('transfer', params, callback);
            },

            getMiningHistory: function(wallet_id,callback){
                var params = {
                    wallet_id: wallet_id
                };
                return this.runCommand('get_mining_history', params, callback);
            },

            validateAddress: function(address){
                if(!this.shouldUseEmulator()){
                    var result =  JSON.parse(Qt_parent['validate_address'](address));
                    if(angular.isDefined(result.error_code)){
                        return result.error_code === 'TRUE' ? true : false;
                    }else{
                        return false;
                    }
                }else{
                    if(address.length == 95){
                        return true;
                    }else{
                        return false;    
                    }
                }
            },

            quitRequest : function() {
                return this.runCommand('on_request_quit', {});
            },

            loadMyOffers : function() {
                if(!returnObject.shouldUseEmulator()){
                    returnObject.get_all_offers(function(data){
                        if(angular.isDefined(data.offers)){
                            $rootScope.offers = data.offers;
                            var my_offers = [];
                            angular.forEach($rootScope.offers,function(item){
                                var result = $filter('filter')($rootScope.safes, item.tx_hash);
                                if(result.length){
                                    my_offers.push(item);
                                }
                            });

                            $rootScope.offers_count = my_offers.length;
                        }
                    });
                }
            },

            reloadCounters : function(){
                $timeout(function(){
                    $rootScope.total_balance = 0;
                    var txCount = 0;
                    angular.forEach($rootScope.safes,function(safe){
                        $rootScope.total_balance += safe.balance;
                        safe.balance_formated = $filter('gulden')(safe.balance);
                        if(safe.history.length){
                            txCount += safe.history.length;
                        }
                    });

                    $rootScope.tx_count = txCount; 

                    
                });
                
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

            runWallet : function(wallet_id) {
                var params = {
                    wallet_id : wallet_id
                };
                return this.runCommand('run_wallet', params);
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
                    'get_all_aliases',
                    'get_all_offers'
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
                                    loaders[request_id] = loader.open(command);
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
                
                status = (status) ? JSON.parse(status) : null;
                param  = (param)  ? JSON.parse(param)  : null;

                console.log(status);
                console.log(param);
                
                var result = {};
                result.status = status;
                result.param = param;

                var request_id = status.request_id;
                var error_code = result.status.error_code;

                if(angular.isDefined(loaders[request_id])) loaders[request_id].close();
                
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
                if (this.shouldUseEmulator()) {
                    emulator.subscribe(command,callback);
                } else {
                    console.log(command);
                    Qt[command].connect(this.callbackStrToObj.bind({callback: callback}));
                }
                
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

        var wallet_id = 0;

        this.getWalletId = function(){
            wallet_id 
            return wallet_id.toString();
        };

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
                "address": "HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVok"+Math.floor(Math.random() * 100),
                "balance": 0,//random_balance(),
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
            if(commandData = this.getData(command,params)){
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
        this.getDaemonState = function(){
            return {
                "daemon_network_state": 2,
                "alias_count" : Math.floor(Math.random()*10),
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
                "last_build_available": "4.23.12.2",
                "last_build_displaymode": 1,

                "out_connections_count": 2,
                "pos_difficulty": "107285151137540",
                "pow_difficulty": "2759454",
                "text_state": "Offline",
                "is_pos_allowed" : true
            };
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
        this.getData = function(command,params){
            var result = false;

            switch (command){
                case 'show_openfile_dialog' : 
                    result = { 
                        "error_code": "OK",
                        "path": "/home/master/Lui/test_wallet.lui"
                    };
                    break;
                case 'get_smart_safe_info' :
                    result = {
                        'restore_key' : 'KFDKTLIUWSEDREBYIOKMTGHJKLGORDDV'
                    };
                    break;
                case 'get_mining_history' :
                    result = {
                        "mined_entries": [
                            {"a":232323, "t": 1429715920},
                            {"a":152244, "t": 1429815920},
                            {"a":132312, "t": 1430755920}
                        ]
                    };
                    break;
                case 'get_alias_coast' : 
                    result = {
                        coast : 10000000000,
                        error_code : "OK"
                    };
                    break;
                case 'restore_wallet' :
                    result = {
                        'wallet_id' : '14'
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
                        "wallet_id" : this.getWalletId(),
                        "wi" : this.getWalletInfo(),
                        "recent_history" : {
                            'history': [
                                {
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
                                    'tx_hash': "514fa3ba101df74bb4ce2c8f8653cd5a9d7c9d5777a4a587878bb5b6cd5954b2",
                                    'unlock_time': 0,
                                    'wid' : params.wallet_id,
                                    'is_anonimous' : true
                                },
                                {
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
                                    'tx_hash': "514fa3ba101df74bb4ce2c8f8653cd5a9d7c9d5777a4a587878bb5b6cd5954b1",
                                    'unlock_time': 0,
                                    'wid' : params.wallet_id,
                                    'is_anonimous' : false
                                }

                            ]
                        },
                    }
                    wallet_id++;
                    break;
                case 'generate_wallet' : 
                    result = {
                        "wallet_id" : 1,
                        "wi" : this.getWalletInfo()
                    }
                    break;
                case 'get_all_offers':
                    result = {
                      "offers": [
                        {
                            "offer_type": 0,
                            "amount_lui": 2300000000,
                            "amount_etc": 2,
                            "target": "Шкаф деревяный",
                            "location_country": "US",
                            "location_city": "ChIJybDUc_xKtUYRTM9XV8zWRD0",
                            "contacts": "test@test.ru,+89876782342",
                            "comment": "Best ever service",
                            "payment_types": "EPS,BC,cash",
                            "expiration_time":3,
                            'timestamp': 1429715920,
                            'tx_hash' : "514fa3ba101df74bb4ce2c8f8653cd5a9d7c9d5777a4a587878bb5b6cd5954b1"
                        }, 
                        {
                            "offer_type": 3,
                            "amount_lui": 4300000000,
                            "amount_etc": 2,
                            "target": "EUR",
                            "location_country": "US",
                            "location_city": "ChIJybDUc_xKtUYRTM9XV8zWRD0",
                            "contacts": "test@test.ru,+89876782342",
                            "comment": "Best ever service",
                            "payment_types": "EPS,BC,Qiwi",
                            "expiration_time":5,
                            'timestamp': 1429715920,
                            'tx_hash' : "514fa3ba101df74bb4ce2c8f8653cd5a9d7c9d5777a4a587878bb5b6cd5954b2"
                        }, 
                        {
                            "offer_type": 2,
                            "amount_lui": 1230000000,
                            "amount_etc": 2,
                            "target": "IDR",
                            "location_country": "US",
                            "location_city": "ChIJybDUc_xKtUYRTM9XV8zWRD0",
                            "contacts": "+89876782342",
                            "comment": "We will rock you",
                            "payment_types": "EPS,BC,Qiwi",
                            "expiration_time":5,
                            'timestamp': 1430715920,
                            'tx_hash' : "514fa3ba101df74bb4ce2c8f8653cd5a9d7c9d5777a4a587878bb5b6cd5954b3"
                        },
                        {
                            "offer_type": 1,
                            "amount_lui": 4300000000,
                            "amount_etc": 2,
                            "target": "Лошадка",
                            "location_country": "US",
                            "location_city": "ChIJybDUc_xKtUYRTM9XV8zWRD0",
                            "contacts": "+89876782342",
                            "comment": "Best ever service",
                            "payment_types": "CSH,BT",
                            "expiration_time":5,
                            'timestamp': 1429715920,
                            'tx_hash' : "514fa3ba101df74bb4ce2c8f8653cd5a9d7c9d5777a4a587878bb5b6cd5954b9"
                        }
                      ]
                    };
                    break;
                case 'get_recent_transfers' :
                    result = {
                        'history' : []

                    }
                    break;
                case 'reset_wallet_password' :
                    result = {
                        'error_code' : 'OK'
                    }
                    break;
                case 'get_wallet_info' : 
                    result = this.getWalletInfo();
                    break;
                case 'get_version' : 
                    result = '4.23.12.1';
                    break;
                case 'update_wallet_info' :
                    result = {
                        wallets  : []
                    };
                    for(var i = 0; i < wallet_id; i++){
                        var wallet = {
                            wallet_id: i.toString(),
                            wi : this.getWalletInfo()
                        }
                        result.wallets.push(wallet);
                    }
                    break;
                case 'update_wallet_status' : 
                    result = {
                        'wallet_id' :this.getWalletId(),
                        'wallet_state' : '2',
                        'is_mining' : false,
                        'balance' : 14300000000,
                        'unlocked_balance' : 4100000000,
                    };
                    break; 
                case 'wallet_sync_progress' :
                    result = {
                        'wallet_id' : this.getWalletId(),
                        'progress' : '50'
                    }
                    break;
                case 'update_daemon_state': 
                    result = this.getDaemonState();
                    break;
                default:
                    result = false;
            }

            return result;
        }
        return this;
    }]);

    

}).call(this);