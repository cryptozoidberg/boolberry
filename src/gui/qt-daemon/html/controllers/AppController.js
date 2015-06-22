(function() {
    'use strict';
    var module = angular.module('app.app',[]);

    module.factory('PassDialogs',['$modal','$rootScope','backend',
        function($modal, $rootScope, backend){
            
            var dialog = function(template, oncancel, onsuccess, canReset){
                $modal.open({
                    templateUrl: template,
                    controller: 'appPassCtrl',
                    size: 'md',
                    windowClass: 'modal fade in',
                    backdrop: false,
                    resolve: {
                        oncancel : function(){
                            return oncancel;
                        },
                        onsuccess : function(){
                            return onsuccess;
                        },
                        canReset : function(){
                            return canReset;
                        }
                    }
                });
            };

            this.generateMPDialog = function(oncancel, onsuccess){
                dialog("views/generate_pass.html", oncancel, onsuccess, false);
            };

            this.requestMPDialog = function(oncancel, onsuccess, canReset){
                if(angular.isUndefined(canReset)){
                    canReset = true;
                }
                dialog("views/request_pass.html", oncancel, onsuccess, canReset);
                $rootScope.settings.security.app_block = true;
            };

            this.storeSecureAppData = function(){
                 console.log('store secure');
                 var safePaths = [];
                 angular.forEach($rootScope.safes,function(item){
                    var safe = {
                        pass: item.pass,
                        path: item.path,
                        name: item.name
                    };
                    safePaths.push(safe);
                 });

                 console.log('Secure Data before save :: ');
                 console.log(safePaths);
                 var result = backend.storeSecureAppData(safePaths, $rootScope.appPass);
                 console.log(result);
            };

            return this;
        }
    ]);


    module.controller('appPassCtrl', ['$scope','backend', '$modalInstance','informer','$rootScope', '$timeout', 'PassDialogs', 'oncancel', 'onsuccess', 'canReset',
        function($scope, backend, $modalInstance, informer, $rootScope, $timeout, PassDialogs, oncancel, onsuccess, canReset) {
            
            $scope.cancel = function(){
                
                if(angular.isFunction(oncancel)){
                    $modalInstance.close();
                    oncancel();
                }
            };

            // On request password cancel when app open:
            // -use master password: false
            // -use master password on money transfer: false

            // On request password cancel when try to remove tick "request password on money transfer"
            // -use master password: true
            // -use master password on money transfer: true

            // On generate password cancel when app open:
            // -use master password: false
            // -use master password on money transfer: false

            // On generate password cancel when try to put tick "use master password":
            // -use master password: false
            // -use master password on money transfer: false

            $scope.canReset = canReset;

            if(angular.isFunction(oncancel)){
                $scope.canCancel = true;
            }else{
                $scope.canCancel = false;                
            }

            $scope.submit = function(appPass){
                console.log(appPass);
                var appData = backend.getSecureAppData({pass: appPass});
                appData = JSON.parse(appData);
                if(angular.isDefined(appData.error_code) && appData.error_code === "WRONG_PASSWORD"){
                    informer.error('Неверный пароль');
                }else{
                    $rootScope.settings.security.app_block = false;
                    $rootScope.appPass = appPass;
                    $modalInstance.close(); 

                    if(angular.isFunction(onsuccess)){
                        onsuccess(appData);
                    }
                        
                }
            };

            $scope.reset = function(){
                $modalInstance.close(); 
                PassDialogs.generateMPDialog(function(){
                    $rootScope.settings.security.is_use_app_pass = false;
                });
            }

            $scope.setPass = function(appPass){
                $rootScope.appPass = appPass;
                PassDialogs.storeSecureAppData();
                $modalInstance.close(); 
            }
        }
    ]);

    

    module.controller('AppController', [
        'backend', '$scope','$timeout', 'loader', '$rootScope','$location', '$filter', '$modal','informer', 'PassDialogs','$interval','Idle',
        function(backend, $scope, $timeout, loader, $rootScope, $location, $filter, $modal, informer, PassDialogs, $interval, Idle) {
        
        // Idle events
        $scope.$on('IdleStart', function() {
        // the user appears to have gone idle
            //informer.info('IdleStart');
        });

        $scope.$on('IdleWarn', function(e, countdown) {
            // follows after the IdleStart event, but includes a countdown until the user is considered timed out
            // the countdown arg is the number of seconds remaining until then.
            // you can change the title or display a warning dialog from here.
            // you can let them resume their session by calling Idle.watch()
            // informer.info('IdleWarn');
        });

        $scope.$on('IdleTimeout', function() {
            // the user has timed out (meaning idleDuration + timeout has passed without any activity)
            // this is where you'd log them
            //informer.info('IdleTimeout');
            PassDialogs.requestMPDialog(
                false,
                function(){ // onsuccess
                    Idle.watch();
                },
                false
            );
        });

        $scope.$on('IdleEnd', function() {
            // the user has come back from AFK and is doing stuff. if you are warning them, you can use this to hide the dialog
            //informer.info('IdleEnd');
        });

        // $scope.$on('Keepalive', function() {
        //     // do something to keep the user's session alive
        //     informer.info('Keepalive');
        // });

        backend.webkitLaunchedScript(); // webkit ready signal to backend

        $rootScope.appPass = false;

        

        $rootScope.deamon_state = {
        	daemon_network_state: 3 // by default "loading core"
        };

        $rootScope.aliases = [];

        $rootScope.unconfirmed_aliases = []; 

        $rootScope.tx_count = 0;

        $rootScope.total_balance = 0;

        $rootScope.offers_count = 0;

        $rootScope.pass_required_intervals = [
            0,
            10, //5 minutes
            600, //10 minutes
            900, //15 minutes
            1800 //30 minutes
        ];

        $rootScope.settings = {
            security: {
                mixin_count: 4,
                is_hide_sender: true,
                is_mixin: false,
                is_pass_required_on_transfer: false,
                is_backup_reminder: false,
                backup_reminder_interval: 0,
                is_use_app_pass: true,
                password_required_interval: $rootScope.pass_required_intervals[1]
            },
            mining: {
                is_block_transfer_when_mining : true,
                ask_confirm_on_transfer_when_mining : false,
                auto_mining : false,
                auto_mining_interval :  0
            },
            app_interface: {
                general: {
                    language : 'ru',
                    font_size : 'medium',
                    is_need_tooltips : false
                },
                on_transfer : {
                    show_recipient_tx_history : false,
                },
                on_sell_buy_guildens : {
                    show_rates : false,
                    show_last_deals : false
                }
            },
            system: {
                app_autoload : false,
                process_finish_remind_onclose : true,
                app_autosave_interval : 0,
                app_donation : false,
                donation_amount : 0,
                default_user_path: '/',
                log_level : 0,
                fav_offers_hash: []
            },
            
            contacts: [],

            widgetColumns : {
                'left': {},
                'right': {}
            } 
        };

        $scope.wallet_info  = {};

        var init = function(){ // app work initialization
            var settings = backend.getAppData(); // get app settinngs
            console.log('GET APP SETTINGS ::');
            console.log(settings);
            // informer.info(JSON.stringify(settings.system.fav_offers_hash));
            if(settings){
                $rootScope.settings = settings;
            } 
            console.log('SHOULD USE APP PASS ::');
            console.log($rootScope.settings.security.is_use_app_pass);
            if($rootScope.settings.security.is_use_app_pass){ // if setings contain "require password"
                if(backend.haveSecureAppData()){
                    PassDialogs.requestMPDialog(
                        function(){
                            $rootScope.settings.security.is_use_app_pass = false;
                            $rootScope.settings.security.is_pass_required_on_transfer = false;
                        }, 
                        function(appData){
                            if(angular.isDefined($rootScope.settings.security.password_required_interval) && $rootScope.settings.security.password_required_interval > 0){
                                Idle.setIdle($rootScope.settings.security.password_required_interval);
                                Idle.watch();
                            }
                            
                            angular.forEach(appData,function(item){
                                backend.openWallet(item.path, item.pass,function(data){
                                    
                                    var new_safe = data.wi;
                                    new_safe.wallet_id = data.wallet_id;
                                    new_safe.name = item.name;
                                    new_safe.pass = item.pass;
                                    new_safe.history = [];

                                    if(angular.isDefined(data.recent_history) && angular.isDefined(data.recent_history.history)){
                                        new_safe.history = data.recent_history.history;
                                    }

                                    $timeout(function(){
                                        $rootScope.safes.push(new_safe); 
                                        backend.runWallet(data.wallet_id);  
                                        backend.reloadCounters(); 
                                        backend.loadMyOffers();
                                    });

                                });
                            });
                        }
                    );
                }else{
                    PassDialogs.generateMPDialog(function(){
                        $rootScope.settings.security.is_use_app_pass = false;
                    });
                }
            }
        };

        var loadingMessage = 'Cеть загружается, или оффлайн. Пожалуйста, подождите...';

        $scope.progress_value = function(){
            var max = $scope.deamon_state.max_net_seen_height - $scope.deamon_state.synchronization_start_height;
            var current = $scope.deamon_state.height - $scope.deamon_state.synchronization_start_height;
            return Math.floor(current*100/max);
        }
        
        $scope.storeAppData = function(){
             console.log('Settings before save :: ');
             console.log($rootScope.settings);
             backend.storeAppData($rootScope.settings);
        };

        $rootScope.closeWallet = function(wallet_id){
            backend.closeWallet(wallet_id, function(data){
                console.log(data);
                for (var i in $rootScope.safes){
                    if($rootScope.safes[i].wallet_id == wallet_id){
                        $rootScope.safes.splice(i,1);
                    }
                }
                backend.reloadCounters();
                backend.loadMyOffers();
                var path = $location.path();
                
                if(path.indexOf('/safe/') > -1){
                    $location.path('/safes');
                }
            });

        };

        $scope.startMining = function(wallet_id){
            backend.startPosMining(wallet_id);
            var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
            if(safe.length){
                safe[0].is_mining_set_manual = true; // flag to understand that we want to prevent auto mining
            }
        }

        $scope.stopMining = function(wallet_id){
            backend.stopPosMining(wallet_id);
            var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
            if(safe.length){
                safe[0].is_mining_set_manual = true; // flag to understand that we want to prevent auto mining
            }
        }

        $scope.resynch = function(wallet_id){
            console.log('RESYNCH WALLET ::' + wallet_id);
            backend.resync_wallet(wallet_id, function(result){
                console.log(result);
            });
        };

        $scope.registerAlias = function(safe){ //TODO check safe data
            
            var modalInstance = $modal.open({
                templateUrl: "views/create_alias.html",
                controller: 'createAliasCtrl',
                size: 'md',
                windowClass: 'modal fade in',
                resolve: {
                    safe: function(){
                        return safe;
                    }
                }
            });
        };

        var loaded = false;
        var alias_count = 0;
        var newVersionShown = [];

        backend.subscribe('update_daemon_state', function(data){// move to run
            if(data.daemon_network_state == 2){
                
                if(!loaded){
                    init();
                    loaded = true;
                }

                var getAliases = function(){
                    backend.getAllAliases(function(data){
                        console.log('ALIASES :: ');
                        console.log(data);
                        if(angular.isDefined(data.aliases) && data.aliases.length){
                            $rootScope.aliases = [];
                            angular.forEach(data.aliases,function(alias){
                                var new_alias = {
                                    alias: alias.alias, 
                                    name: '@'+alias.alias, 
                                    address: alias.address,
                                    comment: alias.comment
                                };
                                $rootScope.aliases.push(new_alias);
                            });    
                        }
                    });
                };

                

                if(alias_count != data.alias_count){
                    alias_count = data.alias_count;
                    getAliases();
                }
                
            }else if(data.daemon_network_state == 4){
                informer.error('Ошибка системы. Для устранения проблемы свяжитесь с разработчиками.');
            }else{
                // if(!li && !loaded){
                //     li = loader.open(loadinMessage);
                // }
            }

            var newVersionModalCtrl = function($scope, $rootScope, $modalInstance) {
                $scope.close = function() {
                  $modalInstance.close();
                };
                //var type = 'success';
                switch($rootScope.deamon_state.last_build_displaymode){
                    case 1:
                        $scope.type = 'success';
                        break;
                    case 2:
                        $scope.type = 'info';
                        break;
                    case 3:
                        $scope.type = 'warning';
                        break;
                    case 4:
                        $scope.type = 'danger';
                        break;
                    default:
                        $scope.type = 'warning';
                }
            };

            // data.last_build_displaymode = 4;
            // data.last_build_available   = '4.0.0.1';
            // var current_v = '4.0.0.0';
            var current_v = backend.getVersion();

            //informer.info(current_v + ' --->>>> '+data.last_build_available);

            $timeout(function(){
            	$rootScope.deamon_state = data;	
                if(data.last_build_displaymode > 0 && current_v != data.last_build_available && newVersionShown.indexOf(data.last_build_available) == -1){
                    var modalInstance = $modal.open({
                        templateUrl: "views/new_version.html",
                        controller: newVersionModalCtrl,
                        size: 'md',
                        windowClass: 'modal fade in',
                        backdrop: false
                    });
                    newVersionShown.push(data.last_build_available);
                }
            });


        });
        
        backend.subscribe('update_wallet_status', function(data){
            var wallet_id = data.wallet_id;
            console.log('UPDATE WALLET STATUS :: ');
            console.log(data);
            var wallet_state = data.wallet_state;
            var is_mining = data.is_mining;
            var balance = data.balance;
            var unlocked_balance = data.unlocked_balance;
            var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
            // informer.info(JSON.stringify(data));
            
            // 1-synch, 2-ready, 3 - error

            if(safe.length){
                $timeout(function(){
                    safe = safe[0];

                    safe.loaded = false;
                    safe.error  = false;

                    safe.is_mining = is_mining;

                    if(wallet_state == 2){ // ready
                        safe.loaded = true;
                        if($rootScope.settings.mining.auto_mining && !safe.is_mining_set_manual){
                            backend.startPosMining(data.wallet_id);
                            safe.is_mining_set_manual = true;
                        }
                    }

                    if(wallet_state == 3){ // error
                        safe.error = true;
                    }   

                    safe.balance = balance;
                    safe.unlocked_balance = unlocked_balance;
                    backend.reloadCounters();
                    
                });
                
            }
        });

        backend.subscribe('quit_requested', function(data){
            console.log('quit_requested');
            console.log($rootScope.appPass);
            if($rootScope.appPass){
                // secure save data
                PassDialogs.storeSecureAppData();
            }
            $scope.storeAppData();
            backend.quitRequest();
        });


        backend.subscribe('wallet_sync_progress', function(data){
            console.log('wallet_sync_progress');
            console.log(data);

            var wallet_id = data.wallet_id;
            var progress = data.progress;

            if($rootScope.safes.length){
                $scope.result = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
                if($scope.result.length){
                    var safe = $scope.result[0];
                    $timeout(function(){
                        safe.progress = progress;    
                    });
                }    
            }
        });

        backend.subscribe('money_transfer', function(data){
            console.log('money_transfer');
            console.log(data);
            if(angular.isUndefined(data.ti)){
                return;
            }

            var wallet_id = data.wallet_id;
            var tr_info   = data.ti;
            var alias = $filter('filter')($rootScope.unconfirmed_aliases,{tx_hash : data.ti.tx_hash});
            
            if(alias.length){ // alias transaction
                alias = alias[0];
                informer.info('Алиас "'+alias.name+'" зарегистрирован');
            }

            var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
            if(safe.length){ // safe transaction
                
                safe = safe[0];

                $timeout(function(){
                    safe.balance = data.balance;
                    safe.unlocked_balance = data.unlocked_balance;
                });
                

                var tr_exists = false;

                angular.forEach(safe.history,function(tr_item, key){
                    if(tr_item.tx_hash == tr_info.tx_hash){
                        // tr_item = tr_info;
                        tr_exists = true;
                        $timeout(function(){
                            safe.history[key] = tr_info;
                        });
                        

                    }
                });

                if(tr_exists){
                    console.log(tr_info.tx_hash+' tr exists');
                }else{
                    console.log(tr_info.tx_hash+' tr does not exist');

                    $timeout(function(){
                        $rootScope.tr_count++;
                        safe.history.unshift(tr_info); // insert new
                    });
                    
                }
                
                backend.reloadCounters();
                
            }else{
                return;
            }

            
        });


    }]);

}).call(this);