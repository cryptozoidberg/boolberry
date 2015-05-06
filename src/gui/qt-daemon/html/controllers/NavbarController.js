(function() {
    'use strict';
    var module = angular.module('app.navbar',[]);

    module.controller('NavbarTopController', ['backend', '$scope','$timeout', 'loader', '$rootScope','$location', '$filter',
        function(backend, $scope, $timeout, loader, $rootScope, $location, $filter) {
        $rootScope.deamon_state = {
        	daemon_network_state: 0
        };

        $scope.wallet_info  = {};

        var loadingMessage = 'Cеть загружается, или оффлайн. Пожалуйста, подождите...';
        var li = loader.open(loadingMessage);

        $scope.progress_value = function(){
            var max = $scope.deamon_state.max_net_seen_height - $scope.deamon_state.synchronization_start_height;
            var current = $scope.deamon_state.height - $scope.deamon_state.synchronization_start_height;
            return Math.floor(current*100/max);
        }

        var appPass = '123'; // TODO

        $scope.getAppData = function(){
            console.log('get');
            var appData = backend.getAppData({pass: appPass});
            appData = JSON.parse(appData);
            console.log(appData);

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

            



            //$rootScope.safes = JSON.parse(data);
        };

        $scope.storeAppData = function(){
             console.log('store');
             var safePaths = [];
             angular.forEach($rootScope.safes,function(item){
                var safe = {
                    pass: item.pass,
                    path: item.path,
                    name: item.name
                };
                safePaths.push(safe);
             });

             backend.storeAppData(safePaths, appPass);

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

        backend.subscribe('update_daemon_state', function(data){// move to run
            // console.log('update_daemon_state');
            // console.log(data);
            if(data.daemon_network_state == 2){
                if(li && angular.isDefined(li)){
                    li.close();
                    li = null;
                }
            }else{
                if(!li){
                    li = loader.open(loadinMessage);
                }
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
                safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
                if(safe.length){
                    safe = safe[0];
                }else{
                    return;
                }
                angular.forEach(wallet_info, function(value,property){
                    safe[property] = value;
                });
                safe.loaded = true;
            });
        });

        backend.subscribe('update_wallet_status', function(data){
            console.log('update_wallet_status');
            console.log(data);
        });


        backend.subscribe('quit_requested', function(data){
            console.log('!!!!before sleep quit_requested!!!!!!');
            $timeout(function(){
            	console.log('!!!!after sleep quit_requested!!!!!!');
		Qt_parent["on_request_quit"]();
		}, 1000);
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