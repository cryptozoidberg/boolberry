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

            $scope.is_currency_offer = function(offer){
                if(offer.offer_type == 2 || offer.offer_type == 3){
                    return true;
                }else{
                    return false;
                }
            }
            $scope.is_good_offer = function(offer){
                if(offer.offer_type == 0 || offer.offer_type == 1){
                    return true;
                }else{
                    return false;
                }
            }
        }
    ]);

    module.controller('addOfferCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location){
            $scope.intervals = [1,3,5,14];

            $scope.offer_types = [
                {key : 0, value: 'Купить товар'},
                {key : 1, value: 'Продать товар'}
            ];

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

            if($rootScope.safes.length){
                $scope.offer.wallet_id = $rootScope.safes[0].wallet_id;
            }

            if($location.path() == '/addOfferSell'){
                $scope.offer.offer_type = 1;
            }else{
                $scope.offer.offer_type = 0;
            }

            

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
                o.amount_etc = 1;
                o.payment_types = '';

                backend.pushOffer(
                    o.wallet_id, o.offer_type, o.amount_lui, o.target, o.location, 
                    o.contacts, o.comment, o.expiration_time, o.fee, o.amount_etc, o.payment_types,
                    function(data){
                        informer.success('Спасибо. Заявка Добавлена');
                    }
                );
            };
        }
    ]);
    // Guilden offer
    module.controller('addGOfferCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','$timeout','market',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location,$timeout,market){
            $scope.intervals = [1,3,5,14];

            console.log();

            $scope.currencies = market.currencies;

            $scope.offer_types = [
                {key : 2, value: 'Покупка гульденов'},
                {key : 3, value: 'Продажа гульденов'}
            ];

            $scope.payment_types = market.paymentTypes;

            $scope.deal_details = [
                "всю сумму целиком",
                "возможно частями"
            ];


            $scope.offer = {
                expiration_time : $scope.intervals[3],
                is_standart : false,
                is_premium : true,
                fee_premium : '6.00',
                fee_standart : '1.00',
                location: {country : '', city: ''},
                contacts: {phone : '', email : ''},
                comment: '',
                currency: $scope.currencies[0].code,
                payment_types: [],
                deal_details: $scope.deal_details[0]
            };

            if($rootScope.safes.length){
                $scope.offer.wallet_id = $rootScope.safes[0].wallet_id;
            }

            if($location.path() == '/addGOfferSell'){
                $scope.offer.offer_type = 3;
            }else{
                $scope.offer.offer_type = 2;
            }

            $scope.changePackage = function(is_premium){
                if(is_premium){
                    $scope.offer.is_standart = $scope.offer.is_premium ? false : true;
                }else{
                    $scope.offer.is_premium = $scope.offer.is_standart ? false : true;
                }
            };

            $scope.recount = function(type){
                
                var toInt = function(value){
                    console.log(value);
                    console.log(parseInt(value));
                    console.log(angular.isNumber(parseInt(value)));
                    
                    if(angular.isNumber(parseInt(value))){
                        if(!isNaN(parseInt(value))){
                            value = parseInt(value);
                        }
                    }else{
                        value = 0;
                    }
                    console.log(value);
                    return value;
                }

                $timeout(function(){
                    switch(type){
                        case 'lui': // amoun lui changes
                            console.log('lui');
                            if(toInt($scope.offer.amount_lui) && toInt($scope.offer.amount_etc)){
                                console.log('1');
                                $scope.offer.rate = $scope.offer.amount_etc / $scope.offer.amount_lui;
                            }else if (toInt($scope.offer.rate)){
                                console.log('2');
                                $scope.offer.amount_etc = toInt($scope.offer.amount_lui) * $scope.offer.rate;
                            }
                            break;
                        case 'target':  // amoun etc changes
                            console.log('target');
                            if(toInt($scope.offer.amount_etc) && toInt($scope.offer.amount_lui)){
                                console.log('1');
                                $scope.offer.rate = $scope.offer.amount_etc / $scope.offer.amount_lui;
                            }else if (toInt($scope.offer.amount_etc) && toInt($scope.offer.rate)){
                                console.log('2');
                                $scope.offer.amount_lui = $scope.offer.amount_etc / $scope.offer.rate;
                            }
                            break;
                        case 'rate': // rate changes
                            console.log('rate');
                            if(toInt($scope.offer.rate) && toInt($scope.offer.amount_lui)){
                                console.log('1');
                                $scope.offer.amount_etc = $scope.offer.rate * $scope.offer.amount_lui;
                            }else if (toInt($scope.offer.rate) && toInt($scope.offer.amount_etc)){
                                console.log('2');
                                $scope.offer.amount_lui = $scope.offer.amount_etc / $scope.offer.rate;
                            }
                            break;
                    }
                });
                
                
            };

            $scope.addOffer = function(offer){
                
                var o = angular.copy(offer);

                
                o.fee = o.is_premium ? o.fee_premium : o.fee_standart;
                o.location = o.location.country + ', ' + o.location.city;
                o.contacts = o.contacts.email + ', ' + o.contacts.phone;
                if(o.payment_type_other) o.payment_types.push(o.payment_type_other);
                o.payment_types = o.payment_types.join(",");
                o.comment = o.comment + (o.comment?' ':'') + o.deal_details;
                o.target = o.currency;
                informer.info(JSON.stringify(o));
                
                backend.pushOffer(
                    o.wallet_id, o.offer_type, o.amount_lui, o.target, o.location, o.contacts, 
                    o.comment, o.expiration_time, o.fee, o.amount_etc, o.payment_types,
                    function(data){
                        informer.success('Спасибо. Заявка Добавлена');
                    }
                );
            };
        }
    ]);

}).call(this);