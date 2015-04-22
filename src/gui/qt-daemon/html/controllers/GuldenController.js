(function() {
    'use strict';
    var module = angular.module('app.guldens',[]);

    module.controller('guldenSendCtrl',['backend','$rootScope','$scope',
        function(backend,$rootScope,$scope){
            $scope.transaction = {
                to: 'HhTZP7Sy4FoDR1kJHbFjzd5gSnUPdpHWHj7Gaaeqjt52KS23rHGa1sN73yZYPt77TkN8VVmHrT5qmBJQGzDLYJGjQpxGRid'
            };
            $scope.send = function(tr){
                backend.transfer(tr.from, tr.to, tr.ammount, tr.fee, tr.comment, function(data){
                    console.log(data);
                });
            };
        }
    ]);

}).call(this);