(function() {
    'use strict';
    var module = angular.module('app.guldens',[]);

    module.controller('guldenSendCtrl',['backend','$rootScope','$scope','informer',
        function(backend,$rootScope,$scope,informer){
            $scope.transaction = {
                to: 'Hc51bpKjQRy1mMMF1zgHVkRbPExx2pwXFFXcaKS93LMmRZtbCVJGMmmRCtVSXz75hvJASHP5Yvu99aH5BjLapN223SDXv6y',
                push_payer: false
            };
            $scope.send = function(tr){
                backend.transfer(tr.from, tr.to, tr.ammount, tr.fee, tr.comment, tr.push_payer, function(data){
                    console.log(data);
                    informer.success('Транзакция поступила в обработку');
                });
            };
        }
    ]);

}).call(this);