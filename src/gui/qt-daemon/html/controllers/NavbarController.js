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
        	daemon_network_state: 3
        };

        $rootScope.aliases = [];
        
        console.log($rootScope.aliases);

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
                donation_amount : 0
            }
        };

        // $rootScope.watch(function(){
        //     console.log('WATCH ROOT SCOPE');
        //     return $rootScope.settings.security.is_use_app_pass;
        // },function($v){
        //     if($v === true){
        //         console.log('APP PASS TRUE');
        //     }else{
        //          console.log('APP PASS FALSE');
        //     }
        // });
       
        // if($rootScope.settings.security.password_required_interval && $rootScope.settings.is_use_app_pass){
        //     console.log('ask pass');
            // $interval(function(){
            //     if(!$rootScope.settings.security.app_block){
            //         PassDialogs.requestMPDialog(false,false,false);
            //     }
            // },$rootScope.settings.password_required_interval);
        // }

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
                                    
                                    var wallet_id = data.wallet_id;
                                    var new_safe = {
                                        wallet_id : wallet_id,
                                        name : item.name,
                                        pass : item.pass
                                    };
                                    $timeout(function(){
                                        $rootScope.safes.push(new_safe);    
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

       

        init();

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

        }

        var loaded = false;

        backend.subscribe('update_daemon_state', function(data){// move to run
            if(data.daemon_network_state == 2){
                
                // if(li && angular.isDefined(li)){
                //     li.close();
                //     li = null;
                // }
                var getAliases = function(){
                    backend.getAllAliases(function(data){
                        console.log('ALIASES :: ');
                        console.log(data);
                        if(angular.isDefined(data.aliases) && data.aliases.length){
                            $rootScope.aliases = [];
                            angular.forEach(data.aliases,function(alias){
                                $rootScope.aliases.push({alias: alias.alias, name: '@'+alias.alias, address: alias.address});
                            });    
                        }
                    });
                };


                if(!loaded){
                    loaded = true;
                    getAliases();
                    $interval(function(){
                        getAliases();
                    },60000); // one minute
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

                if(angular.isUndefined(safe.history)){
                    backend.getRecentTransfers(wallet_id, function(data){
                        if(angular.isDefined(data.unconfirmed)){
                            data.history = data.unconfirmed.concat(data.history);
                        }
                        safe.history = data.history;
                    });
                }
            });
        });

        backend.subscribe('update_wallet_status', function(data){
            console.log('update_wallet_status');
            console.log(data);
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

        backend.subscribe('money_transfer', function(data){
            console.log('money_transfer');
            console.log(data);
            if(angular.isUndefined(data.ti)){
                return;
            }
            var wallet_id = data.wallet_id;
            var tr_info   = data.ti;
            safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
            if(safe.length){
                safe = safe[0];
                safe.balance = data.balance;
                safe.unlocked_balance = data.unlocked_balance;

                if(angular.isUndefined(safe.history)){
                    console.log('no tr history');
                    backend.getRecentTransfers(wallet_id, function(data){
                        if(angular.isDefined(data.unconfirmed)){
                            data.history = data.unconfirmed.concat(data.history);
                        }
                        safe.history = data.history;
                        safe.history.unshift(tr_info);
                    });
                }else{
                    console.log('history exists');
                    //transaction = $filter('filter')(safe.history,{tx_hash : tr_info.tx_hash}); // check if transaction has already in list
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
                        console.log(tr_info.tx_hash+' does not tr exist');
                        safe.history.unshift(tr_info); // insert new
                    }
                    
                }
                
            }else{
                return;
            }
            // angular.forEach(wallet_info, function(value,property){
            //     if(angular.isDefined(safe[property])){
            //         safe[property] = value;
            //     }
            // });
        });


    }]);

}).call(this);