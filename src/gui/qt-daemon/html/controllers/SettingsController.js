(function() {
    'use strict';
    var module = angular.module('app.settings',[]);

    module.controller('settingsCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','PassDialogs','Idle',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location,PassDialogs,Idle){

            $scope.settings = $rootScope.settings;

            $scope.checkPass = function(){
                if($rootScope.settings.security.is_use_app_pass){ // if want to use
                    PassDialogs.generateMPDialog(
                        function(){
                            $rootScope.settings.security.is_use_app_pass = false;
                            $rootScope.settings.security.is_pass_required_on_transfer = false;
                        },
                        function(){
                            Idle.watch();
                        }
                    );
                }else{ //if dont want to use
                    PassDialogs.requestMPDialog(
                        function(){
                            $rootScope.settings.security.is_use_app_pass = true;
                        }, function(){
                            $rootScope.settings.security.is_pass_required_on_transfer = false;
                            Idle.unwatch();
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
                
                if($rootScope.settings.security.password_required_interval > 0){
                    Idle.watch();
                    Idle.setIdle($rootScope.settings.security.password_required_interval);
                }else{
                    Idle.unwatch();
                }
                
            };

            $scope.logLevel = $rootScope.settings.system.log_level;

            $scope.changelogLevel = function(level){
                backend.setLogLevel(level);
                $rootScope.settings.system.log_level = level;
            };

            // $scope.mixCount = $rootScope.settings.security.mixin_count;

            // $scope.mixCount = function(count){
            //     //backend.setLogLevel(level);
            //     $rootScope.settings.security.mixin_count = count;
            // };

        }
    ]);

}).call(this);