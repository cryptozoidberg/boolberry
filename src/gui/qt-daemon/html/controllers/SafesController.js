(function() {
    'use strict';
    var module = angular.module('app.safes',[]);

    module.controller('safeListCtrl',['backend',function(backend){

    }]);

    module.controller('safeDetailsCtrl',['$routeParams','backend','$scope',
        function($routeParams,backend,$scope){
        var wallet_id = parseInt($routeParams.wallet_id);
        $scope.history = {};
        backend.getRecentTransfers(wallet_id, function(data){
            if(angular.isDefined(data.unconfirmed)){
                data.history = data.unconfirmed.concat(data.history);
            }
            $scope.history = data.history;

        });
    }]);

}).call(this);