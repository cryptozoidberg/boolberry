(function() {
    'use strict';
    var module = angular.module('app.navbar',[]);

    module.controller('NavbarTopController', ['backend', '$scope','$timeout', function(backend, $scope, $timeout) {
        $scope.deamon_state = {
        	daemon_network_state: 0
        };

        $scope.wallet_info  = {};
        backend.subscribe('update_daemon_state', function(data){
            // console.log('update deamon state complete');
            $timeout(function(){
            	$scope.deamon_state = data;	
            });
            
        });
        
        backend.subscribe('update_wallet_info', function(data){
            // console.log('update wallet info complete');
            $timeout(function(){
                $scope.wallet_info  = data;
            });
        });
    }]);

}).call(this);