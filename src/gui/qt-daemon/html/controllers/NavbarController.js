(function() {
    'use strict';
    var module = angular.module('app.navbar',[]);

    module.controller('NavbarTopController', ['backend', '$scope','$timeout', 'loader', '$rootScope','$location',
        function(backend, $scope, $timeout, loader, $rootScope, $location) {
        $rootScope.deamon_state = {
        	daemon_network_state: 0
        };

        $scope.wallet_info  = {};

        var loadinMessage = 'Cеть загружается, или оффлайн. Пожалуйста, подождите...';
        var li = loader.open(loadinMessage);

        $scope.progress_value = function(){
            var max = $scope.deamon_state.max_net_seen_height - $scope.deamon_state.synchronization_start_height;
            var current = $scope.deamon_state.height - $scope.deamon_state.synchronization_start_height;
            return Math.floor(current*100/max);
        }

        $rootScope.closeWallet = function(wallet_id){
            console.log('sdfsdf');
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
            $timeout(function(){
                $scope.wallet_info  = data;
            });
        });
    }]);

}).call(this);