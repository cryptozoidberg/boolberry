(function() {
    'use strict';
    var module = angular.module('app.settings',[]);

    module.controller('settingsCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','PassDialogs',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location,PassDialogs){

            $scope.settings = $rootScope.settings;

            $scope.checkPass = function(){
                if($rootScope.settings.security.is_use_app_pass){ // if want to use
                    PassDialogs.generateMPDialog(function(){
                        $rootScope.settings.security.is_use_app_pass = false;
                        $rootScope.settings.security.is_pass_required_on_transfer = false;
                    });
                }else{ //if dont want to use
                    PassDialogs.requestMPDialog(
                        function(){
                            $rootScope.settings.security.is_use_app_pass = true;
                        }, function(){
                            $rootScope.settings.security.is_pass_required_on_transfer = false;
                        },
                        false
                    );
                }
            };

            $scope.requestPass = function(){
                PassDialogs.requestMPDialog(function(){
                    $rootScope.settings.security.is_pass_required_on_transfer = true;
                },false,false);
            };

            $scope.passReqIntervalIndex = $rootScope.pass_required_intervals.indexOf($rootScope.settings.security.password_required_interval);

            $scope.changePassReqInterval = function(index){
                $rootScope.settings.security.password_required_interval = $rootScope.pass_required_intervals[index];
                //console.log($rootScope.settings.security.password_required_interval);
            };

            $scope.logLevel = $rootScope.settings.system.log_level;

            $scope.changelogLevel = function(level){
                backend.setLogLevel(level);
                $rootScope.settings.system.log_level = level;
            };

        }
    ]);

}).call(this);