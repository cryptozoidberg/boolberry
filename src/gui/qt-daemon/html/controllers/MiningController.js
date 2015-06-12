(function() {
    'use strict';
    var module = angular.module('app.mining',[]);

    module.controller('minigHistoryCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){

            //informer.info('mining');
            $scope.mining_history = [];

            var nextDate = new Date().getTime();

            for(var i=1; i!=20; i++){
                var step = Math.random()*10;
                $scope.mining_history.push([new Date(nextDate), step]);
                nextDate += 1000*60*60*24;
            }

        }
    ]);

}).call(this);