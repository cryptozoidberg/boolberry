(function() {
    'use strict';

    var module = angular.module('app.backendServices', [])

    module.factory('backend', ['$interval', '$timeout', 'emulator', 'loader', 'informer',
        function($interval, $timeout, emulator, loader, informer) {
        
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

            transfer: function(from, to, ammount, fee, comment, push_payer, lock_time, callback) {
                 var params = {
                    wallet_id : from,
                    destinations : [
                        {
                            address : to,
                            amount : ammount
                        }
                    ],
                    mixin_count : 0,
                    lock_time : lock_time,
                    payment_id : "",
                    fee : fee,
                    comment: comment,
                    push_payer: push_payer 
                };
                console.log(params);
                return this.runCommand('transfer', params, callback);
            },

            openWallet : function(file, pass, callback) {
                var params = {
                    path: file,
                    pass: pass
                };
                
                return this.runCommand('open_wallet', params, callback);
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
                        
                        if(result.error_code != 'OK'){
                            console.log("API Error for command '"+command+"': " + result.error_code);
                        }else{
                            if(angular.isDefined(callback)){
                                var request_id = result.request_id;
                                loaders[request_id] = loader.open();
                                callbacks[request_id] = callback;
                            }else{
                                return result; // If we didn't pass callback, its mean the function is synch
                            }
                        }
                    }
                }
                
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
                
                loaders[request_id].close();
                console.log('DISPATCH: got result from backend request id = '+request_id);
                console.log(result);
                if(result.status.error_code == 'OK' || result.status.error_code == ''){
                    console.log('run callback');
                    $timeout(function(){
                        console.log('run callback 2');
                        callbacks[request_id](result.param); 
                    },1000);
                    
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
                var actualBackend = ['update_daemon_state', 'update_wallet_info'];

                if (actualBackend.indexOf(command) >= 0) {
                    // *Deep backend layer* fires the event

                    if (this.shouldUseEmulator()) {
                        emulator.subscribe(command,callback);
                    } else {
                        console.log(command);
                        Qt[command].connect(this.callbackStrToObj.bind({callback: callback}));
                    }
                } 
                else {
                    // *This service* fires the event
                    console.log('Subscribe:: command is not supported '+command);
                    this.backendEventSubscribers[command] = callback;
                }
            },

        }

        if (!returnObject.shouldUseEmulator()) {
            console.log('Subscribed on "dispatch"');
            Qt["do_dispatch"].connect(returnObject.backendCallback); // register backend callback
        }
        

        return returnObject;

    }]);

    module.factory('emulator', ['$interval', '$timeout', 'loader',
        function($interval, $timeout, loader){
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
                return Math.random() * 1000000;
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
            var data = {};
            var commandData = false;
            if(commandData = this.getData(command)){
                data = commandData;
            }
            if(angular.isDefined(callback)){
                var loaderInst = loader.open('Run command ' + command + ' params = ' + JSON.stringify(params));
                $timeout(function(){
                    loaderInst.close();
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
                    result = { // why do we need this? I think angular can do it
                        "error_code": "OK",
                        "path": "/home/master/Lui/test_wallet.lui"
                    };
                    break;
                case 'show_savefile_dialog' : 
                    result = { // why do we need this? I think angular can do it
                        "error_code": "OK",
                        "path": "/home/master/Lui/test_wallet_for_save.lui"
                    };
                    break;
                case 'open_wallet' : 
                    result = {
                        "wallet_id" : "1"
                    }
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