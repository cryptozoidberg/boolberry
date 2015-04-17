(function() {
    'use strict';
    var module = angular.module('app.navbar',[]);

    module.controller('NavbarTopController', ['backend', '$scope','$timeout', 'loader', 
        function(backend, $scope, $timeout, loader) {
        $scope.deamon_state = {
        	daemon_network_state: 0
        };

        $scope.wallet_info  = {};

        var loadinMessage = 'Cеть загружается, или оффлайн. Пожалуйста, подождите...';
        var li = loader.open(loadinMessage);

        backend.subscribe('update_daemon_state', function(data){
            console.log(data);
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
            	$scope.deamon_state = data;	
            });
            
        });
        
        
        backend.subscribe('update_wallet_info', function(data){
            $timeout(function(){
                $scope.wallet_info  = data;
            });
        });
    }]);

}).call(this);