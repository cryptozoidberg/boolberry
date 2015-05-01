(function() {
    'use strict';
    var module = angular.module('app.market',[]);

    module.controller('marketCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            console.log('market');
            backend.get_all_offers(function(data){
                console.log(data);
            });
        }
    ]);

    module.controller('addOfferCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            
            $scope.offer = {};

            $scope.addOffer = function(offer){
                console.log(offer);
                backend.push_offer(function(data){
                    console.log('PUSH OFFER RESULT');
                    console.log(data);
                });
            };
        }
    ]);

}).call(this);