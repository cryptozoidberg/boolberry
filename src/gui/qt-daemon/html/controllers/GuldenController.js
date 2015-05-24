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



    module.controller('guldenSendCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','$modal',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location, $modal){
            $scope.transaction = {
                to: '',
                push_payer: $rootScope.settings.security.is_hide_sender,
                is_delay : false,
                lock_time: new Date(),
                fee: '10',
                is_valid_address: false,
                is_mixin : $rootScope.settings.security.is_mixin
            };

            if($routeParams.wallet_id){
                $scope.transaction.from = parseInt($routeParams.wallet_id);
            }
            
            $scope.selectAlias = function(obj){
                var alias = obj.originalObject;
                $scope.transaction.to = alias.address;
                $scope.transaction.is_valid_address = true;
                $scope.transaction.alias = alias.alias;
            }

            $scope.inputChanged = function(str){
                delete $scope.transaction.alias;
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

    module.controller('trHistoryCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
       function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            
            $scope.my_safes = angular.copy($rootScope.safes);
            $scope.my_safes.push({name: 'Все сейфы', wallet_id : -1});
            //$scope.wallet_id = -1;
            
            $scope.tx_history = [];

            //console.log($scope.history);

            angular.forEach($rootScope.safes, function(safe){
                if(angular.isDefined(safe.history)){
                    angular.forEach(safe.history, function(item){
                        item.wallet_id = angular.copy(safe.wallet_id);
                        $scope.tx_history.push(item);    
                    });
                }
            });

            //console.log($scope.history);

            $scope.order = function(key){
                $scope.filtered_history = $filter('orderBy')($scope.filtered_history,key);
            };

            
            

            // $scope.changeWallet = function(wallet_id){
            //     if(wallet_id === -1){
            //         $scope.filtered_history = $scope.history;
            //     }else{
            //         var condition = {wi : { wallet_id: parseInt(wallet_id)}};
            //         $scope.filtered_history = $filter('filter')($scope.history,condition);
            //     }
            // };

            //default filter values
            $scope.filter = {
                tr_type: 'all', //all, in, out
                wallet_id: -1,
                keywords: ''
            };

            $scope.filterChange = function(){
                var f = $scope.filter;
                //informer.info('filtre!');
                $scope.prefiltered_history = angular.copy($scope.tx_history);

                var  message = '<br> filter object = ';
                message += JSON.stringify(f);

                console.log('HISTORY');
                console.log($scope.prefiltered_history);
                //wallet filter
                if(f.wallet_id != -1){
                    var condition = {wi : { wallet_id: parseInt(f.wallet_id)}};
                    $scope.pre_filtered_history = $filter('filter')($scope.pre_filtered_history,condition);
                }

                message += '<br> wallet filter = ';
                message += JSON.stringify($scope.pre_filtered_history);
                //type filter
                if(f.tr_type != 'all'){
                    var is_income = (f.tr_type == 'in') ? true : false;
                    var condition = {wi : { is_income: is_income}};
                    $scope.pre_filtered_history = $filter('filter')($scope.pre_filtered_history,condition);
                }

                message += '<br> type filter = ';
                message += JSON.stringify($scope.pre_filtered_history);
                //keywords
                if(f.keywords != ''){
                    $scope.pre_filtered_history = $filter('filter')($scope.pre_filtered_history,f.keywords);
                }

                message += '<br> keyword filter = ';
                message += JSON.stringify($scope.pre_filtered_history);

                informer.info(message);

                $scope.filtered_history = $scope.pre_filtered_history;
            };

            $scope.filterReset = function(){
                $scope.filtered_history = $scope.tx_history;
                $scope.order('-timestamp');
            };

            $scope.filterReset();
            

            //console.log($scope.filtered_history);
       }
    ]);

}).call(this);