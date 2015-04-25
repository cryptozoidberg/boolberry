(function() {
    'use strict';
    var module = angular.module('app.safes',[]);

    module.controller('safeListCtrl',['backend','$scope','$rootScope',
        function(backend, $rootScope ,$scope){
            
        }
    ]);

    module.controller('safeDetailsCtrl',['$routeParams','backend','$scope','$filter','$location','informer',
        function($routeParams,backend,$scope,$filter,$location,informer){
            var wallet_id = parseInt($routeParams.wallet_id);
            var safe = $filter('filter')($scope.safes,{wallet_id : wallet_id});
            if(safe.length){
                safe = safe[0];
            }else{
                informer.error('Сейф не найден');
                $location.path('/safes');
                return;
            }
            $scope.safe = safe;
            $scope.history = {};
            backend.getRecentTransfers(wallet_id, function(data){
                if(angular.isDefined(data.unconfirmed)){
                    data.history = data.unconfirmed.concat(data.history);
                }
                $scope.history = data.history;

            });
        }
    ]);

    module.controller('safeModals', ['$scope','backend','$modal',function($scope,backend,$modal){
        $scope.openFileDialog = function(){
            var caption = "Please, choose the file";
            var filemask = "*.lui";
            var result = backend.openFileDialog(caption, filemask);
            var path = result.path;
            $scope.openSafeRestoreForm(path);
            
        };

        $scope.openSafeForm = function() {
            var modalInstance = $modal.open({
                templateUrl: "views/safe_new.html",
                controller: 'safeAddCtrl',
                size: 'md',
                windowClass: 'modal fade in out',
            });
            modalInstance.result.then((function() {
                // console.log('Safe form closed');
            }), function() {
                // console.log('Safe form dismissed');
            });
        };

        $scope.openSafeRestoreForm = function(path) {
            var modalInstance = $modal.open({
                templateUrl: "views/safe_restore.html",
                controller: 'safeRestoreCtrl',
                size: 'md',
                windowClass: 'modal fade in out',
                resolve: {
                    path: function(){
                        return path;
                    },
                    safes: function(){
                        return $scope.safes;
                    }
                }
            });
            modalInstance.result.then((function() {
                // console.log('Safe form closed');
            }), function() {
                // console.log('Safe form dismissed');
            });
        };
    }]);

    module.controller('safeAddCtrl', ['$scope','backend', '$modalInstance', '$modal', '$timeout','$rootScope',
        function($scope, backend, $modalInstance, $modal, $timeout, $rootScope) {
            $scope.owl_options  = {
              singleItem: true,
              autoHeight: true,
              navigation: false,
              pagination: false,
              margin: 16,
              mouseDrag: false,
              touchDrag: false,
              callbacks: true,
              smartSpeed: 100,
              autoplayHoverPause: true,
            };

            $scope.safe = {};

            $scope.closeSafeForm = function(){
                $modalInstance.close();
            };

            $scope.saveSafeFile = function(safe) {
                var caption = "Please, choose the file";
                var filemask = "*.lui";
                var result = backend.saveFileDialog(caption, filemask); // TODO digest angular error fix

                backend.generateWallet(result.path,safe.password,function(data){
                    var wallet_id = data.wallet_id;
                    $timeout(function(){
                        $scope.safe.wallet_id = wallet_id;
                    });
                    console.log(data);
                    $modalInstance.close();
                    // backend.getWalletInfo(wallet_id,function(data){
                    //     data.name = $scope.safe.name;
                    //     $rootScope.safes.unshift(data);
                    //     $modalInstance.close();
                    // });
                    
                });    
            };

            $scope.openSmartSafeForm = function() {
                var modalInstance = $modal.open({
                    templateUrl: "views/safe_smartsafe_new.html",
                    controller: 'smartSafeAddCtrl',
                    size: 'md',
                    windowClass: 'modal fade in out',
                });
                modalInstance.result.then((function() {
                    //console.log('Safe form closed');
                }), function() {
                    //console.log('Safe form dismissed');
                });
            };
        }
    ]);

    module.controller('safeRestoreCtrl', ['$scope', 'backend', '$modalInstance', '$modal', '$timeout', 'path', 'safes',
        function($scope, backend, $modalInstance, $modal, $timeout, path, safes) {
            $scope.owl_options  = {
              singleItem: true,
              autoHeight: false,
              navigation: false,
              pagination: false,
              margin: 16,
              mouseDrag: false,
              touchDrag: false,
              callbacks: true,
              smartSpeed: 100,
              autoplayHoverPause: true,
            };
            var filename = path.substr(path.lastIndexOf('/')+1, path.lastIndexOf('.')-1-path.lastIndexOf('/'));

            $scope.safe = {
                path: path,
                name: filename
            };

            $scope.closeSafeForm = function(){
                $modalInstance.close();
            };

            $scope.openSafe = function(safe){
                backend.openWallet(safe.path, safe.pass,function(data){
                    console.log(data);
                    var wallet_id = data.wallet_id;
                    backend.getWalletInfo(wallet_id, function (safe_data){
                        //var new_safe = safe_data.param
                        safe_data.name = safe.name;
                        safe_data.wallet_id = wallet_id;
                        $timeout(function(){
                            safes.unshift(safe_data);    
                        });
                        $modalInstance.close();
                    });
                });
            };
            
        }
    ]);

    module.controller('smartSafeAddCtrl', ['$scope','backend', '$modalInstance',
        function($scope, backend, $modalInstance) {
            $scope.closeSmartSafeForm = function(){
                $modalInstance.close();
            }
        }
    ]);

}).call(this);