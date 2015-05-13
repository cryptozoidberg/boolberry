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
            $scope.intervals = [1,3,5,14];

            $scope.offer = {
                expiration_time : $scope.intervals[3],
                is_standart : false,
                is_premium : true,
                fee_premium : '6.00',
                fee_standart : '1.00',
                location: {country : '', city: ''},
                contacts: {phone : '', email : ''},
                comment: ''
            };

            $scope.changePackage = function(is_premium){
                if(is_premium){
                    $scope.offer.is_standart = $scope.offer.is_premium ? false : true;
                }else{
                    $scope.offer.is_premium = $scope.offer.is_standart ? false : true;
                }
            };

            $scope.addOffer = function(offer){
                var o = angular.copy(offer);
                o.fee = o.is_premium ? o.fee_premium : o.fee_standart;
                o.location = o.location.country + ', ' + o.location.city;
                o.contacts = o.contacts.email + ', ' + o.contacts.phone;

                o.offer_type = 1; //TODO
                
                // 
                backend.pushOffer(
                    o.wallet_id, o.offer_type, o.amount_lui, o.target, o.location, o.contacts, o.comment, o.expiration_time, o.fee,
                    function(data){
                        informer.success('Спасибо. Заявка Добавлена');
                    }
                );
            };
        }
    ]);

}).call(this);