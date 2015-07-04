(function() {
    'use strict';
    var module = angular.module('app.safes',[]);
    
    module.controller('createAliasCtrl',['$scope', '$modalInstance', 'backend', 'safe', '$rootScope',
        function($scope, $modalInstance, backend, safe, $rootScope){
            $scope.alias = {
                name : '',
                fee : '1.76',
                comment: ''
            };

            $scope.close = function(){
                $modalInstance.close();
            };

            $scope.register = function(alias){
                backend.registerAlias(safe.wallet_id, alias.name, safe.address, alias.fee, alias.comment, function(data){
                    $rootScope.unconfirmed_aliases.push(
                        {
                            tx_hash : data.tx_hash,
                            name : alias.name
                        }
                    );
                    
                    $modalInstance.close();
                });
            };
            
        }
    ]);

    module.controller('safeChangePassCtrl',['$scope', '$modalInstance', 'backend', 'safe', '$rootScope',
        function($scope, $modalInstance, backend, safe, $rootScope){
            
            $scope.close = function(){
                $modalInstance.close();
            };

            $scope.safe = {};

            $scope.save = function(safe){
                backend.safeChangePass(safe.wallet_id, alias.name, safe.address, alias.fee, alias.comment, function(data){
                    $rootScope.unconfirmed_aliases.push(
                        {
                            tx_hash : data.tx_hash,
                            name : alias.name
                        }
                    );
                    
                    $modalInstance.close();
                });
            };
        }
    ]);

    module.controller('safeListCtrl',['backend','$scope','$rootScope', '$modal','$interval','$filter',
        function(backend, $scope, $rootScope, $modal,$interval, $filter){
            
            $scope.pieStates = {
                danger: '#D9534F',
                warning: '#F0AD4E',
                info: '#5BC0DE',
                success: '#5CB85C',
            };

            $scope.filter = {
                name: ''
            };

            $scope.filterChange = function(){
                $scope.f_safes = $rootScope.safes;
                if($scope.filter.name.length){
                    $scope.f_safes = $filter('filter')($scope.f_safes, {name: $scope.filter.name});
                }
            };

            $scope.filterChange();

            $scope.safeView = 'list';

            $scope.pieChartoptions = {
                barColor: $scope.pieStates.info, // #5BC0DE info,  #F0AD4E warning,  danger
                trackColor: '#f2f2f2',
                // scaleColor: '#f2f2f2',
                scaleLength: 5,
                lineCap: 'butt',
                lineWidth: 9,
                size: 32,
                rotate: 0,
                animate: {},
                // easing: {}
            };
        }
    ]);

    module.controller('safeDetailsCtrl',['$routeParams','backend','$scope','$filter','$location','informer','$modal',
        function($routeParams,backend,$scope,$filter,$location,informer,$modal){
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
            
            $scope.trDetails = function(item){
                var modalInstance = $modal.open({
                    templateUrl: "views/tr_details.html",
                    controller: 'trDetailsModalCtrl',
                    size: 'md',
                    windowClass: 'modal fade in',
                    resolve: {
                        item: function(){
                            return item;
                        }
                    }
                });
            }

            $scope.row = '-timestramp'; //sort by default

            $scope.order = function(key){
                $scope.row = key;
                $scope.filtered_history = $filter('orderBy')($scope.filtered_history,key);
            };

            $scope.open = function($event,name) {
                $event.preventDefault();
                $event.stopPropagation();
                if(name == 'start'){
                    $scope.opened_start = !$scope.opened_start;
                }else if(name == 'end'){
                    $scope.opened_end = !$scope.opened_end;
                }
            };

            $scope.dateOptions = {
                formatYear: 'yy',
                startingDay: 1
            };

            $scope.format = 'dd/MMMM/yyyy';

            $scope.hide_calendar = true;

            $scope.is_anonim_values = [
                {key : -1 , value: 'любое'},
                {key : 0 , value: 'анонимно'},
                {key : 1 , value: 'неанонимно'},
            ];

            $scope.is_mixin_values = [
                {key : -1 , value: 'не важно'},
                {key : 0 , value: 'микшировано'},
                {key : 1 , value: 'не микшировано'},
            ];

            $scope.interval_values = [
                { key: -1, value : "весь период"},
                { key: 86400, value : "день"},
                { key: 604800, value : "неделя"},
                { key: 2592000, value : "месяц"},
                { key: 5184000, value : "два месяца"},
                { key: -2, value : "другой период"}
            ];

            $scope.hide_calendar = true;

            $scope.filter = {
                tr_type: 'all', //all, in, out
                keywords: '',
                is_anonim : $scope.is_anonim_values[0].key,
                // is_mixin : -1,
                interval : $scope.interval_values[0].key,
                // is_hide_service_tx : false
            };

            $scope.tx_date = {
                // start : new Date(),
                // end: new Date(new Date().getTime() + 604800)
            };

            $scope.filterChange = function(){
                var f = $scope.filter;
                $scope.prefiltered_history = angular.copy($scope.safe.history);

                var  message = '';

                if(f.interval == -2){
                    $scope.hide_calendar = false;

                    if(angular.isDefined($scope.tx_date.start) && angular.isDefined($scope.tx_date.end)){
                        var start = $scope.tx_date.start.getTime()/1000;
                        var end   = $scope.tx_date.end.getTime()/1000 + 60*60*24;

                        var in_range = function(item){
                            console.log(item.timestamp);
                            console.log((start < item.timestamp) && (item.timestamp < end));
                            if((start < item.timestamp) && (item.timestamp < end)){
                                return true;
                            }
                            return false;
                        }

                        if(start < end){
                            $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,in_range);
                        }
                    }
                }else{
                    $scope.hide_calendar = true;
                }

                if(f.interval > 0){
                    var now = new Date().getTime();
                    now = now/1000;
                    var in_interval = function(item){
                        if(item.timestamp > (now - f.interval)){
                            return true;
                        }
                        return false;
                    }
                    $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,in_interval);
                }

                if(f.is_anonim != -1){
                    var is_anonymous = function(item){
                        if(item.remote_address == ''){
                            return true;
                        }
                        return false;
                    }

                    var is_not_anonymous = function(item){
                        return !is_anonymous(item);
                    }

                    if(!f.is_anonim){
                        $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,is_anonymous);
                    }else{
                        $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,is_not_anonymous);
                    }
                }

                if(f.tr_type != 'all'){
                    var is_income = (f.tr_type == 'in') ? true : false;
                    var condition = { is_income: is_income};
                    $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,condition);
                }

                if(f.keywords != ''){
                    $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,f.keywords);
                }


                $scope.filtered_history = $scope.prefiltered_history;
                $scope.order($scope.row);

            };

            $scope.filterChange();

        }
    ]);

    module.controller('trDetailsModalCtrl',['$scope', '$modalInstance', 'item',
        function($scope, $modalInstance, item){
            $scope.item = item;

            $scope.close = function(){
                $modalInstance.close();
            };
        }
    ]);

    module.controller('safeModals', ['$scope','backend','$modal',function($scope,backend,$modal){
        $scope.openFileDialog = function(){
            var caption = "Please, choose the file";
            var filemask = "*.lui";
            var result = backend.openFileDialog(caption, filemask);
            if(typeof result !== 'undefined' && typeof result.path !== 'undefined'){
                var path = result.path;
                $scope.openSafeRestoreForm(path);    
            }
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

        $scope.openSmartSafeRestoreForm = function(path) {
            var modalInstance = $modal.open({
                templateUrl: "views/safe_smart_restore.html",
                controller: 'safeSmartRestoreCtrl',
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

    module.controller('safeAddCtrl', ['$scope','backend', '$modalInstance', '$modal', '$timeout','$rootScope', 'informer',
        function($scope, backend, $modalInstance, $modal, $timeout, $rootScope, informer) {
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

                    var new_safe = data.wi;
                    new_safe.wallet_id = wallet_id;
                    new_safe.name = safe.name;
                    new_safe.pass = safe.password;
                    new_safe.history = [];
                    
                    $timeout(function(){
                        $scope.safe = new_safe;
                        $scope.safe.fileSaved = true;
                        $rootScope.safes.unshift(new_safe);
                        backend.runWallet(data.wallet_id);
                        backend.reloadCounters();
                    });
                    
                });    
            };

            

            $scope.startUseSafe = function(){
                $modalInstance.close();
            };

            $scope.openSmartSafeForm = function() {
                var modalInstance = $modal.open({
                    templateUrl: "views/safe_smartsafe_new.html",
                    controller: 'smartSafeAddCtrl',
                    size: 'md',
                    windowClass: 'modal fade in out',
                    resolve : {
                        safe : function(){
                            return $scope.safe;
                        }
                    }
                });
                modalInstance.result.then((function() {
                    //console.log('Safe form closed');
                }), function() {
                    //console.log('Safe form dismissed');
                });
            };
        }
    ]);

    module.controller('safeSmartRestoreCtrl', ['$scope', 'backend', '$modalInstance', '$modal', '$timeout', 'path', 'safes', '$rootScope', 'informer',
        function($scope, backend, $modalInstance, $modal, $timeout, path, safes, $rootScope, informer) {
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

            $scope.safe = {
                restore_key : '',  
                pass : '',
                path : '',
            }

            $scope.saveWalletFile = function(){
                var caption = "Please, choose the file";
                var filemask = "*.lui";
                var result;
                if(result = backend.saveFileDialog(caption, filemask)){
                    $scope.safe.path = result.path;
                }
            }

            $scope.changeRestoreKey = function(safe){
                backend.restoreWallet(safe.path,safe.pass,safe.restore_key,function(data){
                    var new_safe = data.wi;
                    new_safe.wallet_id = data.wallet_id;
                    new_safe.name = safe.name;
                    new_safe.pass = safe.pass;
                    new_safe.history = [];

                    if(angular.isDefined(data.recent_history) && angular.isDefined(data.recent_history.history)){
                        new_safe.history = data.recent_history.history;
                    }
                    
                    $modalInstance.close();
                    $timeout(function(){
                        $rootScope.safes.unshift(new_safe); 
                        backend.runWallet(data.wallet_id);
                        backend.reloadCounters();
                        backend.loadMyOffers();
                    });
                });
                    
                
            }
        }
    ]);

    module.controller('safeRestoreCtrl', ['$scope', 'backend', '$modalInstance', '$modal', '$timeout', 'path', 'safes', '$rootScope',
        function($scope, backend, $modalInstance, $modal, $timeout, path, safes, $rootScope) {
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
            var folder = path.substr(0,path.lastIndexOf('/'));

            $rootScope.settings.system.default_user_path = folder;

            backend.storeAppData($rootScope.settings);

            $scope.safe = {
                path: path,
                name: filename
            };

            $scope.closeSafeForm = function(){
                $modalInstance.close();
            };

            $scope.openSafe = function(safe){
                backend.openWallet(safe.path, safe.pass,function(data){
                    
                    var new_safe = data.wi;
                    new_safe.wallet_id = data.wallet_id;
                    new_safe.name = safe.name;
                    new_safe.pass = safe.pass;
                    new_safe.history = [];

                    if(angular.isDefined(data.recent_history) && angular.isDefined(data.recent_history.history)){
                        new_safe.history = data.recent_history.history;
                    }
                    
                    $modalInstance.close();
                    $timeout(function(){
                        safes.unshift(new_safe); 
                        backend.runWallet(data.wallet_id);
                        backend.reloadCounters();   
                        backend.loadMyOffers();
                    });

                });
            };
            
        }
    ]);

    module.controller('smartSafeAddCtrl', ['$scope','backend', '$modalInstance', 'safe', 'informer',
        function($scope, backend, $modalInstance, safe, informer) {
            
            //$scope.safe = safe;

            var data = backend.getSmartSafeInfo(safe.wallet_id);

            $scope.restore_key = data.restore_key;

            $scope.closeSmartSafeForm = function(){
                $modalInstance.close();
            }
        }
    ]);

}).call(this);