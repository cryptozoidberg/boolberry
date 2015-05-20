(function() {
    'use strict';
    var module = angular.module('app.navbar',[]);

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

            this.generateMPDialog = function(oncancel){
                dialog("views/generate_pass.html", oncancel, false, false);
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
                $modalInstance.close();
                console.log('cancel');
                if(angular.isFunction(oncancel)){
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

            // $scope.cancelGenerate = function(){
            //     $modalInstance.close(); 
            //     $rootScope.settings.security.is_use_app_pass = false;
            // };

            $scope.canReset = canReset;


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

    

    module.controller('NavbarTopController', [
        'backend', '$scope','$timeout', 'loader', '$rootScope','$location', '$filter', '$modal','informer', 'PassDialogs','$interval',
        function(backend, $scope, $timeout, loader, $rootScope, $location, $filter, $modal, informer, PassDialogs, $interval) {
        
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
            30000, //5 minutes
            60000, //10 minutes
            90000, //15 minutes
            180000 //30 minutes
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
                password_required_interval: $rootScope.pass_required_intervals[0]
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
                default_user_path: '/'
            }
        };

        $scope.wallet_info  = {};

        var init = function(){ // app work initialization
            var settings = backend.getAppData(); // get app settinngs
            console.log('GET APP SETTINGS ::');
            console.log(settings);
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
                        
                            angular.forEach(appData,function(item){
                                backend.openWallet(item.path, item.pass,function(data){
                                    backend.runWallet(data.wallet_id);
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
                                        backend.reloadCounters(); 
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
                var path = $location.path();
                
                if(path.indexOf('/safe/') > -1){
                    $location.path('/safes');
                }
            });

        };

        $scope.startMining = function(wallet_id){
            backend.startPosMining(wallet_id);
            
        }

        $scope.stopMining = function(wallet_id){
            backend.stopPosMining(wallet_id);
            
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
            $timeout(function(){
            	$rootScope.deamon_state = data;	
            });
        });
        
        backend.subscribe('update_wallet_info', function(data){
            return;
            

            angular.forEach(data.wallets,function (wallet){
                console.log('update_wallet_info');
                console.log(data);
                var wallet_id = wallet.wallet_id;
                var wallet_info = wallet.wi;
                var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
                if(safe.length){
                    safe = safe[0];
                }else{
                    return;
                }
                angular.forEach(wallet_info, function(value,property){
                    safe[property] = value;
                });



                safe.loaded = true;
                safe.balance_formated = $filter('gulden')(safe.balance);

                // if(angular.isUndefined(safe.history)){
                //     backend.getRecentTransfers(wallet_id, function(data){
                //         if(angular.isDefined(data.unconfirmed)){
                //             data.history = data.unconfirmed.concat(data.history);
                //         }
                //         safe.history = data.history;
                //         //informer.info('tr count before wallet update '+$rootScope.tr_count);
                //         $rootScope.tr_count = $rootScope.tr_count + safe.history.length;
                //         //informer.info('tr count after wallet update '+$rootScope.tr_count);
                //     });
                // }
            });
            // recountTotalBalance();
        });


        backend.subscribe('update_wallet_status', function(data){
            var wallet_id = data.wallet_id;
            console.log('UPDATE WALLET STATUS :: ');
            console.log(data);
            var wallet_state = data.wallet_state;
            var is_mining = data.is_mining;
            var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
            // informer.info('UPDATE WALLET STATUS' + wallet_state);
            
            // 1-synch, 2-ready, 3 - error

            if(safe.length){
                $timeout(function(){
                    safe = safe[0];

                    safe.loaded = false;
                    safe.error  = false;
                    safe.is_mining = is_mining;


                    if(wallet_state == 2){ // ready
                        safe.loaded = true;
                    }

                    if(wallet_state == 3){ // error
                        safe.error = true;
                    }   
                    
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

        var wait = 0;

        backend.subscribe('wallet_sync_progress', function(data){
            console.log('wallet_sync_progress');
            console.log(data);

            var wallet_id = data.wallet_id;
            var progress = data.progress;

            // $timeout(function(){
                safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
                if(safe.length){
                    safe = safe[0];
                        safe.progress = progress;
                        if(safe.progress == 100){
                            safe.loaded = true;
                        }
                }
            // },wait);
            // wait+=100;
            
        });

        backend.subscribe('money_transfer', function(data){
            console.log('money_transfer');
            console.log(data);
            if(angular.isUndefined(data.ti)){
                return;
            }
            var wallet_id = data.wallet_id;
            var tr_info   = data.ti;

            alias = $filter('filter')($rootScope.unconfirmed_aliases,{tx_hash : data.ti.tx_hash});

            if(alias.length){ // alias transaction
                alias = alias[0];
                informer.info('Алиас "'+alias.name+'" зарегистрирован');
                return;
            }

            safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
            if(safe.length){ // safe transaction
                safe = safe[0];
                safe.balance = data.balance;
                safe.unlocked_balance = data.unlocked_balance;

                // if(angular.isUndefined(safe.history)){
                //     console.log('no tr history');
                //     backend.getRecentTransfers(wallet_id, function(data){
                //         if(angular.isDefined(data.unconfirmed)){
                //             data.history = data.unconfirmed.concat(data.history);
                //         }
                //         safe.history = data.history;
                //         $rootScope.tr_count++;
                        
                //         safe.history.unshift(tr_info);
                //     });
                // }else{
                    console.log('history exists');

                    var tr_exists = false;
                    angular.forEach(safe.history,function(tr_item, key){
                        if(tr_item.tx_hash == tr_info.tx_hash){
                            // tr_item = tr_info;
                            safe.history[key] = tr_info;
                            tr_exists = true;

                        }
                    });
                    if(tr_exists){
                        console.log(tr_info.tx_hash+' tr exists');
                    }else{
                        console.log(tr_info.tx_hash+' tr does not exist');
                        $rootScope.tr_count++;
                        //informer.info('tr count after on money transfer (history) ' + $rootScope.tr_count);
                        safe.history.unshift(tr_info); // insert new
                    }
                    
                // }
                // backend.recountTotalBalance();
                
            }else{
                return;
            }
            
        });


    }]);

}).call(this);