(function() {
    'use strict';
    var module = angular.module('app.settings',[]);

    module.controller('settingsCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','PassDialogs',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location,PassDialogs){
            console.log('settings');
            $scope.settings = $rootScope.settings;

            $scope.checkPass = function(){
                if($rootScope.settings.security.is_use_app_pass){ // if want to use
                    PassDialogs.generateMPDialog();
                }else{ //if dont want to use
                    PassDialogs.requestMPDialog(false, function(){
                        $rootScope.settings.security.is_pass_required_on_transfer = false;
                    });
                }
            };

            $scope.requestPass = function(){ //TODO merge with checkPass
                PassDialogs.requestMPDialog(false);
            };

        }
    ]);

}).call(this);