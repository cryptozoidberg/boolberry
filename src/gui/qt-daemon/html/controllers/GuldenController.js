(function() {
    'use strict';
    var module = angular.module('app.guldens',[]);

    
    module.controller('appPassOnTransferCtrl', ['$scope','backend', '$modalInstance','informer','$rootScope', '$timeout', 'tr',
        function($scope, backend, $modalInstance, informer, $rootScope, $timeout, tr) {
            $scope.tr = tr;
            $scope.need_pass = $rootScope.settings.security.is_pass_required_on_transfer;
            $scope.confirm = function(appPass){
                console.log(appPass);
                console.log($scope.app_path);
                if($scope.need_pass){
                    var appData = backend.getSecureAppData({pass: appPass});
                    appData = JSON.parse(appData);
                    if(angular.isDefined(appData.error_code) && appData.error_code === "WRONG_PASSWORD"){
                        informer.error('Неверный пароль');
                        return;
                    }
                }
                backend.makeSend(tr);
                $modalInstance.close();
            };

            $scope.close = function(){
                $modalInstance.close();
            }
        }
    ]);



    module.controller('guldenSendCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','$modal','txHistory',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location, $modal, txHistory){
            $scope.transaction = {
                to: '',
                push_payer: $rootScope.settings.security.is_hide_sender,
                is_delay : false,
                lock_time: new Date(),
                fee: '10',
                is_valid_address: false,
                is_mixin : $rootScope.settings.security.is_mixin
            };

            $scope.txHistory = txHistory.reloadHistory();

            if($routeParams.wallet_id){
                $scope.transaction.from = parseInt($routeParams.wallet_id);
            }

            if($routeParams.address){
                $scope.transaction.is_valid_address = true;
                $scope.transaction.to = $routeParams.address;
            }
            
            $scope.selectAlias = function(obj){
                var alias = obj.originalObject;
                $scope.transaction.to = alias.address;
                $scope.transaction.is_valid_address = true;
                // $scope.transaction.alias = alias.alias;
                return alias.address;
            }

            $scope.inputChanged = function(str){
                // delete $scope.transaction.alias;
                if(str.indexOf('@') != 0){
                    if(backend.validateAddress(str)){
                        $scope.transaction.is_valid_address = true;
                        $scope.transaction.to = str;
                    }else{
                        $scope.transaction.is_valid_address = false;
                        $scope.transaction.to = '';
                    }
                }else{
                    $scope.transaction.to = '';
                    $scope.transaction.is_valid_address = false;
                }
            }

            $scope.send = function(tr){
                console.log(tr);
                $modal.open({
                    templateUrl: "views/tr_confirm.html",
                    controller: 'appPassOnTransferCtrl',
                    size: 'md',
                    windowClass: 'modal fade in',
                    backdrop: false,
                    resolve: {
                        tr: function(){
                            return tr;
                        }
                    }

                });
                
            };

            $scope.validate_address = function(){
                if($scope.transaction.to.length == 95){
                    $scope.transaction.is_valid_address = true;
                }else{
                    $scope.transaction.is_valid_address = false;
                }
            };

            $scope.disabled = function(date, mode) {
                return ( mode === 'day' && ( date.getDay() === 0 || date.getDay() === 6 ) );
            };

            $scope.open = function($event) {
                $event.preventDefault();
                $event.stopPropagation();
                $scope.opened = !$scope.opened;
            };

            $scope.getLockTime = function() {
                return $scope.lock_time;
            };

            $scope.tp_open = function($event) {
                $event.preventDefault();
                $event.stopPropagation();
                $scope.tp_opened = !$scope.tp_opened;
            };

            $scope.dateOptions = {
                formatYear: 'yy',
                startingDay: 1
            };

            $scope.formats = ['dd-MMMM-yyyy', 'yyyy/MM/dd', 'dd.MM.yyyy', 'shortDate'];
            $scope.format = $scope.formats[0];
        }
    ]);

    module.controller('trHistoryCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','txHistory',
       function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location, txHistory){
            
            $scope.my_safes = angular.copy($rootScope.safes);
            $scope.my_safes.unshift({name: 'Все сейфы', wallet_id : -1});

            $scope.contact = false;

            var init = function(){
                if($routeParams.contact_id){
                    var contact = $filter('filter')($rootScope.settings.contacts, {id: $routeParams.contact_id});
                    $scope.contact = contact[0];
                    $scope.tx_history = txHistory.contactHistory($scope.contact);
                }else{
                    $scope.tx_history = txHistory.reloadHistory();
                }
            };

            init();

            $scope.$watch(
                function(){
                    return $rootScope.safes;
                },
                function(){
                    init();
                },
                true
            );

            $scope.row = '-timestramp'; //sort by default

            $scope.order = function(key){
                $scope.row = key;
                $scope.filtered_history = $filter('orderBy')($scope.filtered_history,key);
            };

            // scope.disabled = function(date, mode) {
            //     return ( mode === 'day' && ( date.getDay() === 0 || date.getDay() === 6 ) );
            // };

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

            //default filter values
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
                wallet_id: -1,
                keywords: '',
                is_anonim : $scope.is_anonim_values[0].key,
                is_mixin : -1,
                interval : $scope.interval_values[0].key
            };

            $scope.tx_date = {
                // start : new Date(),
                // end: new Date(new Date().getTime() + 604800)
            };

            $scope.filterChange = function(){
                var f = $scope.filter;
                $scope.prefiltered_history = angular.copy($scope.tx_history);

                var  message = '';

                if(f.wallet_id != -1){
                    message += 'wallet filter';
                    var condition = {wallet_id: parseInt(f.wallet_id)};
                    $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,condition);
                }

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

                if(f.is_mixin != -1){
                    var condition = f.is_mixin ? { is_anonymous: true} : { is_anonymous: false}; // is_anonymous its actualy is_mixin 
                    $scope.prefiltered_history = $filter('filter')($scope.prefiltered_history,condition);
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

            $scope.filterReset = function(){
                $scope.filtered_history = $scope.tx_history;
                $scope.order($scope.row);
            };

            $scope.filterReset();

       }
    ]);

}).call(this);


