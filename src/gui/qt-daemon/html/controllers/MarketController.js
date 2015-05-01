(function() {
    'use strict';
    var module = angular.module('app.market',[]);

    module.controller('marketCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            console.log('market');
            backend.get_all_offers(function(data){
                if(angular.isDefined(data.offers)){
                    console.log(data.offers[0]);
                    $scope.offers = data.offers;
                }
                console.log(data);
            });
        }
    ]);

    module.controller('addOfferCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            
            $scope.offer = {};

            $scope.addOffer = function(offer){
                // o = offer;
                backend.push_offer(function(data){
                    informer.success('Спасибо. Заявка Добавлена');
                });
            };
        }
    ]);

}).call(this);