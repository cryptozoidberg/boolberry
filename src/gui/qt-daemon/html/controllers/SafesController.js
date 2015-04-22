(function() {
    'use strict';
    var module = angular.module('app.safes',[]);

    module.controller('safeListCtrl',['backend',function(backend){

    }]);

    module.controller('safeDetailsCtrl',['$routeParams','backend',function($routeParams,backend){
        var wallet_id = $routeParams.wallet_id;
        console.log(wallet_id);
        backend.getRecentTransfers(wallet_id, function(data){
            console.log(data);

        });
    }]);

}).call(this);