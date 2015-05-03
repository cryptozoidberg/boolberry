(function() {
    'use strict';
    var module = angular.module('app.guldens',[]);

    module.controller('guldenSendCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            $scope.transaction = {
                to: '',
                push_payer: false,
                is_delay : false,
                lock_time: new Date(),
                fee: '0.01',
                is_valid_address: false
            };

            if($routeParams.wallet_id){
                $scope.transaction.from = $routeParams.wallet_id+'';
            }

            $scope.send = function(tr){
                var TS = 0;
                var mkTS = 0;
                if(tr.is_delay){
                    mkTS = tr.lock_time.getTime(); //miliseconds timestamp
                    TS = Math.floor(mkTS/1000); // seconds timestamp    
                }
                
                backend.transfer(tr.from, tr.to, tr.ammount, tr.fee, tr.comment, tr.push_payer, TS, function(data){
                    console.log(data);
                    informer.success('Транзакция поступила в обработку');
                    //$location.path('/safe/'+tr.from);

                });
            };

            $scope.validate_address = function(){
                if($scope.transaction.to.length == 95){
                    $scope.transaction.is_valid_address = true;
                }else{
                    $scope.transaction.is_valid_address = false;
                }
            }

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
                console.log('dfgdfg');
            };

            $scope.dateOptions = {
                formatYear: 'yy',
                startingDay: 1
            };

            $scope.formats = ['dd-MMMM-yyyy', 'yyyy/MM/dd', 'dd.MM.yyyy', 'shortDate'];
            $scope.format = $scope.formats[0];
        }
    ]);

}).call(this);