(function() {
    'use strict';
    var module = angular.module('app.settings',[]);

    module.controller('settingsCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            console.log('settings');
            $scope.settings = $rootScope.settings;
        }
    ]);

}).call(this);