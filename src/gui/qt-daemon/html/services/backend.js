(function() {
    'use strict';

    var module = angular.module('app.backendServices', [])

    module.factory('backend', ['$interval', '$timeout', 'emulator',
        function($interval, $timeout, emulator) {
        
        var callbacks = {};

        var returnObject =  {
            
            runCommand : function(command, params, callback) {
                if(this.shouldUseEmulator()){
                    emulator.runCommand(command, params, callback);
                }else{
                    var action = Qt_parent[command];
                    if(typeof action === undefined){
                        console.log("API Error for command '"+command+"': command not found in Qt object");
                    }else{
                        var resultString = action(JSON.stringify(params)); 
                        var result = (resultString === '') ? {} : JSON.parse(resultString);
                        
                        console.log('API command ' + command +' call result: '+JSON.stringify(result));
                        
                        if(result.error_code != 'OK'){
                            console.log("API Error for command '"+command+"': " + result.error_code);
                        }else{
                            
                            var request_id = result.request_id
                            callbacks[request_id] = callback;
                        }
                    }
                }
                
            },

            backendCallback: function(status, param) {
                status = (status) ? JSON.parse(status) : null;
                param  = (param)  ? JSON.parse(param)  : null;
                result = {
                    status: status,
                    param:  param
                };
                var request_id = status.request_id;
                console.log('got result from backend request id = '+request_id);
                console.log(callbacks[request_id]);
                callbacks[request_id](result);
            },

            openFiledialog : function(caption, filemask) {
                var params = {
                    caption: caption, 
                    filemask: filemask
                };

                return this.runCommand('show_openfile_dialog',params);
            },

            shouldUseEmulator : function() {
                var use_emulator = (typeof Qt == 'undefined');
                console.log("UseEmulator: " + use_emulator);
                return use_emulator;
            },

            openWallet : function(file, pass, callback) {
                var params = {
                    path: file,
                    pass: pass
                };
                return this.runCommand('open_wallet', params, callback);
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

        if(!returnObject.shouldUseEmulator()){
            Qt.dispatch.connect(returnObject.backendCallback); // register backend callback    
        }
        

        return returnObject;

    }]);

    module.factory('emulator', ['$interval',function($interval){
        var callbacks = {};

        return {
            runCommand : function(command, params, callback) {
                var data = {};
                switch(command){
                    case 'open_wallet':
                        data = {
                            wallet_id : 1
                        };
                        break;
                }
                if(angular.isDefined(callback)){
                    callback(data);
                }else{
                    console.log('fire event '+command);
                }
                
            },
            subscribe : function(command, callback) {
                var data = {};

                if(angular.isDefined(this.dataBank[command])){
                    data = this.dataBank[command];
                }

                var interval = $interval(function(){
                    callback(data);
                },1000);
            },
            dataBank : {
                'update_daemon_state': {
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
                }
            }
        }
    }]);

}).call(this);