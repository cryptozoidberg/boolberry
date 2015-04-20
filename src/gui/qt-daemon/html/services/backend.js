(function() {
    'use strict';

    var module = angular.module('app.backendServices', [])

    module.factory('backend', ['$interval', '$timeout', 'emulator', 'loader',
        function($interval, $timeout, emulator, loader) {
        
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

            openWallet : function(file, pass, callback) {
                var params = {
                    path: file,
                    pass: pass
                };
                
                return this.runCommand('open_wallet', params, callback);
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
                        console.log("API Error for command '"+command+"': command not found in Qt object");
                    }else{
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
                console.log('DISPATCH: got result from backend request id = ' + request_id);
                status = (status) ? JSON.parse(status) : null;
                param  = (param)  ? JSON.parse(param)  : null;
                result = {
                    status: status,
                    param:  param
                };
                var request_id = status.request_id;
                loaders[request_id].close();
                console.log('DISPATCH: got result from backend request id = '+request_id);
                console.log(callbacks[request_id]);
                if(result.status.error_code == 'OK'){
                    $timer(function(){
                        callbacks[request_id](result);
                    },1000);
                    
                }else{
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

        var get_wallet_info = function() {
            
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


        return {
            runCommand : function(command, params, callback) {
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
                
            },
            subscribe : function(command, callback) {
                var data = {};
                var commandData = false;
                if(commandData = this.getData(command)){
                    data = commandData;
                }

                var interval = $interval(function(){
                    //data.daemon_network_state = Math.floor(Math.random()*3);
                    callback(data);
                },3000);
            },
            getData : function(command){
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
                        result = get_wallet_info();
                        break;
                    
                    case 'update_daemon_state': 
                        result = {
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
                        break;
                    default:
                        result = false;
                }

                return result;
            }
        }
    }]);

    module.factory('loader',['$modal',function($modal){
        var ModalInstanceCtrl = function($scope, $modalInstance) {

            $scope.cancel = function() {
              $modalInstance.dismiss('cancel');
            };
        };


        return {
            open: function(message){
               
                if(!angular.isDefined(message)){
                    message = 'Пожалуйста, подождите...';
                }

                var modalHtml = '<div class="modal-header btn btn-primary btn-block">';
                modalHtml += '<button type="button" class="close" data-dismiss="modal" ng-click="cancel()">';
                modalHtml += '<span aria-hidden="true">.</span><span class="sr-only">Close</span>';
                modalHtml += '</button>';
                modalHtml += '<h4 class="modal-title text-center">Загружается...</h4>';
                modalHtml += '</div>';
                modalHtml += '<div class="modal-body">';
                modalHtml += '<span class="ifOnlineText loading text-primary">';
                modalHtml += '<i class="fa fa-2x fa-circle-o-notch fa-spin"></i> '+message;
                modalHtml += '</span>';
                modalHtml += '</div>';

                return $modal.open({
                  template: modalHtml,
                  controller: ModalInstanceCtrl,
                  backdrop: false,
                  size: 'sm'
                });
            },
            close: function(instance){
                if(angular.isDefined(instance)){
                    instance.close();
                }
            }
        }

    }]);

}).call(this);