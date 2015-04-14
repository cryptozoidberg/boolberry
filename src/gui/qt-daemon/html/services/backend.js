(function() {
    'use strict';

    var module = angular.module('app.backendServices', [])
    
    module.factory('backend', ['$interval', '$timeout',
        function($interval, $timeout) {
        
        var daemonStateInfo = {};
        var daemonStateText = 'Loading';
        var emulator        = emulator;
        var callbacks       = {};

        var returnObject =  {
            
            runCommand : function(command, params, callback) {

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

                    // if (this.shouldUseEmulator()) {
                    //     this.emulator.eventCallbacks[command] = callback;
                    // } else {
                    //     console.log(command);
                        Qt[command].connect(this.callbackStrToObj.bind({callback: callback}));
                    // }
                } 
                // else {
                //     // *This service* fires the event
                //     this.backendEventSubscribers[command] = callback;
                // }
            },

        }

        Qt.dispatch.connect(returnObject.backendCallback); // register backend callback

        return returnObject;

    }]);

    module.factory('emulator', [function() {
        return new Emulator();
    }]);

    function Backend(emulator, $rootScope, $timeout) {
        this.daemonStateInfo = {};
        this.daemonStateText = 'Loading';
        this.emulator = emulator;

        var backend = this;

        this.openSafe = function() {
            var params = {caption: "test", filemask: "*.lui2"};
            console.log(JSON.stringify(params));
            var result = Qt_parent['show_openfile_dialog'](JSON.stringify(params));
            console.log(result);
            return;
        };

        // Fires at the end of parent Backend function
        this.init = function() {
            this.subscribe('update_daemon_state', function(data) {
                // upper right corner indicator
                backend.daemonStateText = data.text_state;

                // info widget
                backend.daemonStateInfo = data;
                // $timeout(function(){
                //     $rootScope.$apply();    
                // });
                
            });
            this.subscribe('update_wallet_info',function(data) {
                //console.log(data);
            });
        };

        this.subscribe = function(command, callback) {
            var actualBackend = ['update_daemon_state', 'update_wallet_info'];

            if (actualBackend.indexOf(command) >= 0) {
                // *Deep backend layer* fires the event

                if (this.shouldUseEmulator()) {
                    this.emulator.eventCallbacks[command] = callback;
                } else {
                    console.log(command);
                    Qt[command].connect(this.callbackStrToObj.bind({callback: callback}));
                }
            } else {
                // *This service* fires the event
                this.backendEventSubscribers[command] = callback;
            }
        };

        this.callbackStrToObj = function(str) {
            var obj = $.parseJSON(str);
            this.callback(obj);
        };

        this.shouldUseEmulator = function() {
            var use_emulator = (typeof Qt == 'undefined');
            console.log("UseEmulator: " + use_emulator);
            return use_emulator;
        };

        this.init();
    }

    function Emulator() {
        this.eventCallbacks = [];
        var emulator = this;

        setInterval(function() {
            var data = {};
            if (emulator.eventCallbacks['update_daemon_state']) {
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

                emulator.eventCallbacks['update_daemon_state'](data);
            }
            if (emulator.eventCallbacks['update_wallet_info']) {
                data = {result : true}
                emulator.eventCallbacks['update_wallet_info'](data);
            }
        }, 5000, emulator);
    }

}).call(this);