(function() {
    'use strict';
    var module = angular.module('app.market',[]);

    module.controller('marketCtrl',['CONFIG', 'backend','$rootScope','$scope','informer','$routeParams','$filter','$location','market','$timeout', 'gProxy', '$http',
        function(CONFIG,backend,$rootScope,$scope,informer,$routeParams,$filter,$location, market, $timeout, gProxy, $http){
            
            $scope.config = CONFIG;

            var is_currency_offer = function(offer){
                if(offer.offer_type == 2 || offer.offer_type == 3){
                    return true;
                }
                return false;
            };

            var is_goods_offer = function(offer){
                if(offer.offer_type == 0 || offer.offer_type == 1){
                    return true;
                }
                return false;
            };

            if(angular.isDefined($rootScope.settings.system.fav_offers_hash)){
                $scope.fav_offers_hash = $rootScope.settings.system.fav_offers_hash;
            }else{
                $scope.fav_offers_hash = [];
            }

            var is_fav = function(offer){
                if($scope.fav_offers_hash.indexOf(offer.tx_hash) > -1){
                    return true;
                }
                return false;
            };

            $scope.is_fav = is_fav;

            $scope.toggleFav = function(offer){
                
                var index = $scope.fav_offers_hash.indexOf(offer.tx_hash);
                // alert(index);
                if(index > -1){
                    $scope.fav_offers_hash.splice(index,1);
                }else{
                    $scope.fav_offers_hash.push(offer.tx_hash);
                }
                $rootScope.settings.system.fav_offers_hash = $scope.fav_offers_hash;
                loadFavorites();
                $scope.favOffersFilterChange();
            };

            var loadFavorites = function(){
                $scope.fav_offers = $filter('filter')($rootScope.offers, is_fav);
                $scope.fav_currency_offers = $filter('filter')($scope.fav_offers, is_currency_offer);
                $scope.fav_goods_offers    = $filter('filter')($scope.fav_offers, is_goods_offer);
                $scope.f_fav_currency_offers = $scope.fav_currency_offers;
                $scope.f_fav_goods_offers = $scope.fav_goods_offers;
            };

            $scope.cancelOffer = function(offer){
                
                var safe = $filter('filter')($rootScope.safes, offer.tx_hash);
                if(safe.length){
                    safe = safe[0];
                    var data = backend.cancelOffer(safe.wallet_id, offer.tx_hash , function(result){
                        // {"success":true,"tx_blob_size":775,"tx_hash":"82dc10c91658ff425c8aaf1809e5259ce5269f280429689e195cfbb4be4f6e65"}
                        // informer.info(result.tx_hash);
                        angular.forEach($scope.f_my_goods_offers,function(item, key){
                            if(item.tx_hash == offer.tx_hash){
                                $scope.f_my_goods_offers.splice(key, 1);
                            }
                        });
                        
                    });
                }else{
                    informer.error('Не найден сейф, с которого было опубликовано предложение');
                }

                
            };

            $scope.getEditLink = function(offer) {
                var link = 'market'; //by default
                switch(offer.offer_type){
                    case 0:
                        link = 'addOfferBuy/' + offer.tx_hash;
                        break;
                    case 1:
                        link = 'addOfferSell/' + offer.tx_hash;
                        break;
                    case 2:
                        link = 'addGOfferBuy/' + offer.tx_hash;
                        break;
                    case 3:
                        link = 'addGOfferBuy/' + offer.tx_hash;
                        break;
                }
                return link;
            };

            $scope.order = function(row, target){
                switch(target){
                    case 'currency':
                        $scope.f_currency_offers = $filter('orderBy')($scope.f_currency_offers, row);
                        break;

                    case 'goods':
                        $scope.f_goods_offers = $filter('orderBy')($scope.f_goods_offers, row);
                        break;

                    case 'my_currency':
                        $scope.f_my_currency_offers = $filter('orderBy')($scope.f_my_currency_offers, row);
                        break;

                    case 'my_goods':
                        $scope.f_my_goods_offers = $filter('orderBy')($scope.f_my_goods_offers, row);
                        break;

                    case 'fav_currency':
                        $scope.f_fav_currency_offers = $filter('orderBy')($scope.f_fav_currency_offers, row);
                        break;
                        
                    case 'fav_goods':
                        $scope.f_fav_goods_offers = $filter('orderBy')($scope.f_fav_goods_offers, row);
                        break;
                }
            };

            if(angular.isUndefined($rootScope.gplaces)){
                $rootScope.gplaces = {};
            }

            if(angular.isUndefined($rootScope.countryList)){
                $http.get('all.json').then(
                    function(res){
                      $rootScope.countryList = res.data;
                    }
                );
            }

            $scope.$watch(
                function(){
                    return $rootScope.offers;
                },
                function(){
                    
                    $rootScope.offers = $filter('orderBy')($rootScope.offers,'-timestamp');

                    $scope.my_offers = [];

                    angular.forEach($rootScope.offers,function(item){
                        
                        var placeId = item.location_city;

                        var not_found = 'City not found';

                        if( angular.isUndefined($rootScope.gplaces[placeId]) ) {
                            $rootScope.gplaces[placeId] = {name : 'Loading...'};
                            gProxy.getDetails(placeId,function(place){
                                $timeout(function(){
                                    $rootScope.gplaces[placeId] = place;    
                                });
                            });
                        }
                        // else{
                        //     $rootScope.gplaces[placeId] = {name : not_found};
                        // }
                        
                        var result = $filter('filter')($rootScope.safes, item.tx_hash);

                        if(result.length){
                            $scope.my_offers.push(item);
                            
                        }
                    });

                    $scope.currency_offers = $filter('filter')($rootScope.offers, is_currency_offer);
                    $scope.goods_offers = $filter('filter')($rootScope.offers, is_goods_offer);
                    
                    $scope.f_currency_offers = $scope.currency_offers; // filtered currency offers by default
                    $scope.f_goods_offers    = $scope.goods_offers; // filtered goods offers by default

                    $rootScope.offers_count = $scope.my_offers.length;

                    $scope.my_currency_offers = $filter('filter')($scope.my_offers, is_currency_offer);
                    $scope.my_goods_offers    = $filter('filter')($scope.my_offers, is_goods_offer);
                    $scope.f_my_currency_offers = $scope.my_currency_offers;
                    $scope.f_my_goods_offers = $scope.my_goods_offers;

                    loadFavorites();
                },
                true
            );
            

            $scope.saveMyOffers = function(){
                var caption = "Please, choose the path";
                var filemask = "*.txt";
                var result = backend.saveFileDialog(caption, filemask); 
                if(typeof result !== 'undefined' && typeof result.path !== 'undefined'){
                    var path = result.path;
                    backend.storeFile(path, $scope.my_offers);
                    // informer.info(path);
                    // informer.info(JSON.stringify());
                }
            };

            $scope.goods_interval_values = [
                { key: -1, value : "не важно"},
                { key: 3600, value : "час"},
                { key: 10800, value : "3 часа"},
                { key: 86400, value : "день"},
                { key: 172800, value : "два дня"},
                { key: 259200, value : "три дня"},
                { key: 604800, value : "неделя"},
                // { key: 604801, value : "больше недели"}, // ?
                { key: -2, value : "другой период"}
            ];

            $scope.payment_types = market.paymentTypes;
            $scope.currencies = angular.copy(market.currencies);
            $scope.currencies.unshift({code: 'Не важно', title : 'Не важно'});
            $scope.hide_calendar   = true;
            $scope.cf_hide_calendar = true;
            $scope.gf_opened = {};
            $scope.gf_opened.start   = false;
            $scope.gf_opened.end   = false;
            $scope.gf_opened.cur_start   = false;
            $scope.gf_opened.cur_end   = false;

            $scope.dateOptions = {
                formatYear: 'yy',
                startingDay: 1
            };

            $scope.format = 'dd/MM/yyyy';

            $scope.gf_open = function($event,name) {
                $event.preventDefault();
                $event.stopPropagation();
                $timeout(function(){
                    if(name == 'start'){
                        $scope.gf_opened.start = !$scope.gf_opened.start;
                    }else if(name == 'end'){
                        $scope.gf_opened.end = !$scope.gf_opened.end;
                    }else if(name == 'cur_start'){
                        $scope.gf_opened.cur_start = !$scope.gf_opened.cur_start;
                    }else if(name == 'cur_end'){
                        $scope.gf_opened.cur_end = !$scope.gf_opened.cur_end;
                    }    
                });
            };

            // CURRENCY FILTER

            $scope.currency_filter = {
                offer_type: 'all', //all, in, out
                keywords: '',
                country_keywords: '',
                city_keywords: '',
                interval : $scope.goods_interval_values[0].key,
                currency : $scope.currencies[0].code,
                payment_types : []
            };

            $scope.currency_offer_date = {};

            $scope.currencyFilterChange = function(){
                $scope.pf_currency_offers = angular.copy($scope.currency_offers); // prefiltered goods offers

                var cf = $scope.currency_filter; 

                if(cf.interval == -2){
                    $scope.cf_hide_calendar = false;

                    if(angular.isDefined($scope.currency_offer_date.start) && angular.isDefined($scope.currency_offer_date.end)){
                        var start = $scope.currency_offer_date.start.getTime()/1000;
                        var end   = $scope.currency_offer_date.end.getTime()/1000 + 60*60*24;

                        var currency_in_range = function(item){
                            if((start < item.timestamp) && (item.timestamp < end)){
                                return true;
                            }
                            return false;
                        }

                        if(start < end){
                            $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers,currency_in_range);
                        }
                    }
                }else{
                    $scope.cf_hide_calendar = true;

                }

                if(cf.interval > 0){
                    var now = new Date().getTime();
                    now = now/1000;

                    var in_interval = function(item){
                        
                        if(item.timestamp > (now - cf.interval)){
                            return true;
                        }
                        return false;
                    }
                    
                    $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers,in_interval);
                }

                if(cf.keywords != ''){
                    $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers,cf.keywords);
                }

                if(cf.country_keywords != ''){
                    $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers, {location : cf.country_keywords});
                }

                if(cf.city_keywords != ''){
                    $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers, {location : cf.city_keywords});
                }

                if(cf.offer_type != 'all'){
                    var condition = { offer_type: (cf.offer_type == 'sell') ? 2 : 3};
                    $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers,condition);
                }

                if(cf.currency != $scope.currencies[0].code){
                    $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers,{target : cf.currency});
                }

                if(cf.payment_types.length){
                    var in_types = function(item){
                        var result = false;
                        angular.forEach(item.payment_types.split(','),function(type){
                            if(cf.payment_types.indexOf(type) > -1){
                                result =  true;
                            }    
                        });
                        
                        return result;
                    }

                    $scope.pf_currency_offers = $filter('filter')($scope.pf_currency_offers,in_types);
                }

                $scope.f_currency_offers = $scope.pf_currency_offers;
            };

            // GOODS FILTER

            

            $scope.goods_filter = {
                offer_type: 'all', //all, in, out
                keywords: '',
                country_keywords: '',
                city_keywords: '',
                interval : $scope.goods_interval_values[0].key
            };

            $scope.goods_offer_date = {};

            $scope.goodsFilterChange = function(){
                $scope.pf_goods_offers = angular.copy($scope.goods_offers); // prefiltered goods offers

                var gf = $scope.goods_filter; 

                if(gf.interval == -2){
                    $scope.hide_calendar = false;

                    if(angular.isDefined($scope.goods_offer_date.start) && angular.isDefined($scope.goods_offer_date.end)){
                        var start = $scope.goods_offer_date.start.getTime()/1000;
                        var end   = $scope.goods_offer_date.end.getTime()/1000 + 60*60*24;

                        var goods_in_range = function(item){
                            if((start < item.timestamp) && (item.timestamp < end)){
                                return true;
                            }
                            return false;
                        }

                        if(start < end){
                            $scope.pf_goods_offers = $filter('filter')($scope.pf_goods_offers,goods_in_range);
                        }
                    }
                }else{
                    $scope.hide_calendar = true;

                }

                if(gf.interval > 0){
                    var now = new Date().getTime();
                    now = now/1000;

                    var in_interval = function(item){
                        
                        if(item.timestamp > (now - gf.interval)){
                            return true;
                        }
                        return false;
                    }
                    
                    $scope.pf_goods_offers = $filter('filter')($scope.pf_goods_offers,in_interval);
                }

                if(gf.keywords != ''){
                    $scope.pf_goods_offers = $filter('filter')($scope.pf_goods_offers,gf.keywords);
                }

                if(gf.country_keywords != ''){
                    $scope.pf_goods_offers = $filter('filter')($scope.pf_goods_offers, {location : gf.country_keywords});
                }

                if(gf.city_keywords != ''){
                    $scope.pf_goods_offers = $filter('filter')($scope.pf_goods_offers, {location : gf.city_keywords});
                }

                if(gf.offer_type != 'all'){
                    var condition = { offer_type: (gf.offer_type == 'sell') ? 1 : 0};
                    $scope.pf_goods_offers = $filter('filter')($scope.pf_goods_offers,condition);
                }

                $scope.f_goods_offers = $scope.pf_goods_offers;
            };

            //MY OFFERS FILTER

            $scope.myOffersView = 'currency'; // tab by default

            $scope.my_offers_filter = {
                offer_type: 'all', //all, in, out
                keywords: ''
            };

            $scope.myOffersFilterChange = function(){
                $scope.pf_my_currency_offers = angular.copy($scope.my_currency_offers);
                $scope.pf_my_goods_offers    = angular.copy($scope.my_goods_offers);

                var mof = $scope.my_offers_filter; 

                if(mof.keywords != ''){
                    $scope.pf_my_currency_offers = $filter('filter')($scope.pf_my_currency_offers,mof.keywords);
                    $scope.pf_my_goods_offers = $filter('filter')($scope.pf_my_goods_offers,mof.keywords);
                }

                if(mof.offer_type != 'all'){
                    var condition = { offer_type: (mof.offer_type == 'sell') ? 1 : 0};
                    $scope.pf_my_goods_offers = $filter('filter')($scope.pf_my_goods_offers,condition);

                    var condition = { offer_type: (mof.offer_type == 'sell') ? 2 : 3};
                    $scope.pf_my_currency_offers = $filter('filter')($scope.pf_my_currency_offers,condition);
                }

                $scope.f_my_goods_offers = $scope.pf_my_goods_offers;
                $scope.f_my_currency_offers = $scope.pf_my_currency_offers;
            };

            //FAVORITE OFFERS FILTER

            $scope.favOffersView = 'currency'; // tab by default

            $scope.fav_offers_filter = {
                offer_type: 'all', //all, in, out
                keywords: ''
            };

            $scope.favOffersFilterChange = function(){
                $scope.pf_fav_currency_offers = angular.copy($scope.fav_currency_offers);
                $scope.pf_fav_goods_offers    = angular.copy($scope.fav_goods_offers);

                var fof = $scope.fav_offers_filter; 

                if(fof.keywords != ''){
                    $scope.pf_fav_currency_offers = $filter('filter')($scope.pf_fav_currency_offers,fof.keywords);
                    $scope.pf_fav_goods_offers = $filter('filter')($scope.pf_fav_goods_offers,fof.keywords);
                }

                if(fof.offer_type != 'all'){
                    var condition = { offer_type: (fof.offer_type == 'sell') ? 1 : 0};
                    $scope.pf_fav_goods_offers = $filter('filter')($scope.pf_fav_goods_offers,condition);

                    var condition = { offer_type: (fof.offer_type == 'sell') ? 2 : 3};
                    $scope.pf_fav_currency_offers = $filter('filter')($scope.pf_fav_currency_offers,condition);
                }

                $scope.f_fav_goods_offers = $scope.pf_fav_goods_offers;
                $scope.f_fav_currency_offers = $scope.pf_fav_currency_offers;
            };

            
        }
    ]);

    module.controller('addOfferCtrl',['CONFIG', 'backend','$rootScope','$scope','informer','$routeParams','$filter','$location','$http','$timeout','$q','$window', 'gProxy','market',
        function(CONFIG, backend,$rootScope,$scope,informer,$routeParams,$filter,$location, $http, $timeout, $q, $window, gProxy, market){
           if(angular.isUndefined($rootScope.offers)){
                $rootScope.offers = [];
            }

            $scope.intervals = [ 
                1, 3, 5, 14
                // {seconds: 60*60*24,    days: 1},
                // {seconds: 60*60*24*3,  days: 3},
                // {seconds: 60*60*24*5,  days: 5},
                // {seconds: 60*60*24*14, days: 14}
            ];

            

            $scope.offer_types = [
                {key : 0, value: 'Купить товар'},
                {key : 1, value: 'Продать товар'}
            ];

            $scope.offer = {
                expiration_time : $scope.intervals[3],
                is_standart : false,
                is_premium : true,
                fee_premium : CONFIG.premium_fee,
                fee_standart : CONFIG.standart_fee,
                location: {country : '', city: ''},
                contacts: {phone : '', email : ''},
                comment: '',
                bonus: '',
                initial_country: '',
                initial_city: ''
            };

            var offer_to_edit = false;
            $scope.is_edit    = false;
            if($routeParams.offer_hash){
                $scope.is_edit = true;
                var search = $filter('filter')($rootScope.offers, {tx_hash : $routeParams.offer_hash});
                if(search.length){
                    offer_to_edit = search[0];
                }
            }

            $scope.selectedCity = function(obj){
                 if(angular.isDefined(obj)){
                    var o = obj.originalObject;
                    $scope.offer.location_city = o.place_id;
                }
            }

            $scope.searchAPI = function(userInputString, timeoutPromise) {

                return $q(function(resolve, reject) {
                    var country = '';
                    if(angular.isDefined($scope.offer.location_country) && $scope.offer.location_country.length){
                        country = $scope.offer.location_country;
                    }

                    $timeout(function(){
                        gProxy.getPredictions(userInputString,'ru','ru',function(data){
                            resolve({data: data});
                        });
                    });
                });

            }



            if(angular.isUndefined($rootScope.countryList)){
                $http.get('all.json').then(
                    function(res){
                      $rootScope.countryList = res.data;
                    }
                );
            }

            $scope.cityOptionsAC = {
                types: ['(cities)']
            }

            $scope.selectedCountry = function(obj){
                if(angular.isDefined(obj)){
                    var o = obj.originalObject;
                    $scope.cityOptionsAC.componentRestrictions = {country: o.alpha2Code};
                    $scope.offer.location_country = o.alpha2Code;
                    $scope.offer.autocomplete_city = '';
                    $scope.offer.location_city = '';
                }

                $scope.$watch(function(){
                    return $scope.offer.autocomplete_city;
                },
                function($value){
                    if(angular.isDefined($value.place_id)){
                        $scope.offer.location_city = $value.place_id;
                    }
                },
                true);
               
            };

            

            if($location.path() == '/addOfferSell'){
                $scope.offer.offer_type = 1;
            }else{
                $scope.offer.offer_type = 0;
            }

            var last_offer = false;

            angular.forEach($filter('orderBy')($rootScope.offers, '-timestamp'),function(item){
                if(item.offer_type == $scope.offer.offer_type){
                    var result = $filter('filter')($rootScope.safes, item.tx_hash);

                    if(result.length){
                        last_offer = item;
                    }
                }
                
            });

            var offer_to_fill = offer_to_edit ? offer_to_edit : (last_offer ? last_offer : false);
            if(offer_to_fill){
                //$scope.offer = offer_to_fill;

                $scope.offer.is_standart      = offer_to_fill.is_standart;
                $scope.offer.expiration_time  = offer_to_fill.expiration_time;
                $scope.offer.location_city    = offer_to_fill.location_city;
                $scope.offer.location_country = offer_to_fill.location_country;

                var country = $filter('filter')($rootScope.countryList, {alpha2Code: $scope.offer.location_country});
                if(country.length){
                    country = country[0];
                    $scope.offer.initial_country = country.name;
                }

                gProxy.getDetails($scope.offer.location_city, function(data){
                    $timeout(function(){
                        $scope.offer.initial_city = data.name;
                    });
                    
                });

                $scope.offer.amount_lui = $filter('gulden')(offer_to_fill.amount_lui, false);
                //$scope.offer.amount_etc = offer_to_fill.amount_etc;
                $scope.offer.comment    = offer_to_fill.comment;
                $scope.offer.target     = offer_to_fill.target;

                if(offer_to_fill.payment_types.length){
                    $scope.offer.payment_types = offer_to_fill.payment_types.split(',');

                    angular.forEach($scope.offer.payment_types,function(value){
                        if(!market.paymentTypes.hasOwnProperty(value)){
                            $scope.offer.payment_type_other = value;
                        }
                    });  
                }
                

                var contacts = offer_to_fill.contacts.split(',');
                
                if(angular.isDefined(contacts[0])){
                    $scope.offer.contacts.email = contacts[0];
                }

                if(angular.isDefined(contacts[1])){
                    $scope.offer.contacts.phone = contacts[1];
                }

                //$scope.recount('target');
            }

            if($rootScope.safes.length){
                $scope.offer.wallet_id = $rootScope.safes[0].wallet_id;
            }

            

            

            $scope.changePackage = function(is_premium){
                if(is_premium){
                    $scope.offer.is_standart = $scope.offer.is_premium ? false : true;
                }else{
                    $scope.offer.is_premium = $scope.offer.is_standart ? false : true;
                }
            };

            $scope.addOffer = function(offer){
                
                if(!$scope.offerForm.$valid){
                    return;
                }

                var o = angular.copy(offer);
                o.fee = o.is_premium ? o.fee_premium : o.fee_standart;
                o.contacts = o.contacts.email + ', ' + o.contacts.phone;
                o.amount_etc = 1;
                o.payment_types = '';

                // informer.info(o.expiration_time);
                if($routeParams.offer_hash){
                    var safe = $filter('filter')($rootScope.safes, $routeParams.offer_hash);
                    if(safe.length){
                        backend.pushUpdateOffer(
                            safe[0].wallet_id, $routeParams.offer_hash, o.offer_type, o.amount_lui, o.target, o.location_city, o.location_country, o.contacts, 
                            o.comment, o.expiration_time, o.fee, o.amount_etc, o.payment_types, o.bonus, '',
                            function(data){
                                informer.success('Спасибо. Заявка Обновлена');
                            }
                        );
                    }else{
                        informer.error('Не найден сейф, с которого было опубликовано предложение');
                    }
                }else{
                    backend.pushOffer(
                        o.wallet_id, o.offer_type, o.amount_lui, o.target, o.location_city, o.location_country, 
                        o.contacts, o.comment, o.expiration_time, o.fee, o.amount_etc, o.payment_types, '', '',
                        function(data){
                            informer.success('Спасибо. Заявка Добавлена');
                        }
                    );
                }
            };
        }
    ]);
    // Guilden offer
    module.controller('addGOfferCtrl',['CONFIG', 'backend','$rootScope','$scope','informer','$routeParams','$filter','$location','$timeout','market', '$http','$q','gProxy',
        function(CONFIG, backend,$rootScope,$scope,informer,$routeParams,$filter,$location,$timeout,market,$http,$q,gProxy){
            if(angular.isUndefined($rootScope.offers)){
                $rootScope.offers = [];
            }

            $scope.intervals = [
                1, 3, 5, 14
                // {seconds: 60*60*24,    days: 1},
                // {seconds: 60*60*24*3,  days: 3},
                // {seconds: 60*60*24*5,  days: 5},
                // {seconds: 60*60*24*14, days: 14}
            ];

            $scope.currencies = market.currencies;

            $scope.offer_types = [
                {key : 2, value: 'Покупка гульденов'},
                {key : 3, value: 'Продажа гульденов'}
            ];

            $scope.deal_details = [
                {key : 'ALL',   value: 'всю сумму целиком'},
                {key : 'PARTS', value: 'возможно частями'}
            ];

            $scope.selectedCity = function(obj){
                 if(angular.isDefined(obj)){
                    var o = obj.originalObject;
                    $scope.offer.location_city = o.place_id;
                }
            }

            $scope.searchAPI = function(userInputString, timeoutPromise) {

                return $q(function(resolve, reject) {

                    $timeout(function(){
                        var country = '';
                        if(angular.isDefined($scope.offer.location_country) && $scope.offer.location_country.length){
                            country = $scope.offer.location_country;
                        }
                        gProxy.getPredictions(userInputString,country,'ru',function(data){
                            resolve({data: data});
                        });
                    });
                });

            }

            $scope.bonus_type = 'G';

            $scope.payment_types = market.paymentTypes;

            

            if(angular.isUndefined($rootScope.countryList)){
                $rootScope.countryList = [];
                $http.get('all.json').then(
                    function(res){
                      $rootScope.countryList = res.data;
                    }
                );
            }

            $scope.cityOptionsAC = {
                types: ['(cities)']
            }

            $scope.selectedCountry = function(obj){
                if(angular.isDefined(obj)){
                    var o = obj.originalObject;
                    $scope.cityOptionsAC.componentRestrictions = {country: o.alpha2Code};
                    $scope.offer.location_country = o.alpha2Code;
                    $scope.offer.autocomplete_city = '';
                    $scope.offer.location_city = '';
                }

                $scope.$watch(function(){
                    return $scope.offer.autocomplete_city;
                },
                function($value){
                    if(angular.isDefined($value.place_id)){
                        $scope.offer.location_city = $value.place_id;
                    }
                },
                true);
               
            };

            $scope.offer = {
                expiration_time : $scope.intervals[3],
                is_standart : false,
                is_premium : true,
                fee_premium : CONFIG.premium_fee,
                fee_standart : CONFIG.standart_fee,
                bonus: '',
                location: {country : '', city: ''},
                contacts: {phone : '', email : ''},
                comment: '',
                currency: $scope.currencies[0].code,
                payment_types: [],
                deal_details: $scope.deal_details[0].key,
                initial_country: '',
                initial_city: ''
            };

            var offer_to_edit = false;
            $scope.is_edit = false;
            if($routeParams.offer_hash){
                $scope.is_edit = true;
                var search = $filter('filter')($rootScope.offers, {tx_hash : $routeParams.offer_hash});
                if(search.length){
                    offer_to_edit = search[0];
                }
            }

            $scope.changeCountryInput = function(str){
                $scope.offer.location_country = '';
            };

            if($rootScope.safes.length){
                $scope.offer.wallet_id = $rootScope.safes[0].wallet_id;
            }

            if($location.path() == '/addGOfferSell'){
                $scope.offer.offer_type = 3;
            }else{
                $scope.offer.offer_type = 2;
            }

            var last_offer = false;

            angular.forEach($filter('orderBy')($rootScope.offers, '-timestamp'),function(item){
                if(item.offer_type == $scope.offer.offer_type){
                    var result = $filter('filter')($rootScope.safes, item.tx_hash);

                    if(result.length){
                        last_offer = item;
                    }
                }
                
            });

            $scope.changePackage = function(is_premium){
                if(is_premium){
                    $scope.offer.is_standart = $scope.offer.is_premium ? false : true;
                }else{
                    $scope.offer.is_premium = $scope.offer.is_standart ? false : true;
                }
            };

            $scope.recount = function(type){
                
                var toInt = function(value){
                    
                    if(angular.isNumber(parseInt(value))){
                        if(!isNaN(parseInt(value))){
                            value = parseInt(value);
                        }
                    }else{
                        value = 0;
                    }
                    return value;
                }

                $timeout(function(){
                    switch(type){
                        case 'lui': // amoun lui changes
                            if(toInt($scope.offer.amount_lui) && toInt($scope.offer.amount_etc)){
                                $scope.offer.rate = $scope.offer.amount_etc / $scope.offer.amount_lui;
                            }else if (toInt($scope.offer.rate)){
                                $scope.offer.amount_etc = toInt($scope.offer.amount_lui) * $scope.offer.rate;
                            }
                            break;
                        case 'target':  // amoun etc changes
                            if(toInt($scope.offer.amount_etc) && toInt($scope.offer.amount_lui)){
                                $scope.offer.rate = $scope.offer.amount_etc / $scope.offer.amount_lui;
                            }else if (toInt($scope.offer.amount_etc) && toInt($scope.offer.rate)){
                                $scope.offer.amount_lui = $scope.offer.amount_etc / $scope.offer.rate;
                            }
                            break;
                        case 'rate': // rate changes
                            if(toInt($scope.offer.rate) && toInt($scope.offer.amount_lui)){
                                $scope.offer.amount_etc = $scope.offer.rate * $scope.offer.amount_lui;
                            }else if (toInt($scope.offer.rate) && toInt($scope.offer.amount_etc)){
                                $scope.offer.amount_lui = $scope.offer.amount_etc / $scope.offer.rate;
                            }
                            break;
                    }
                });
                
                
            };

            var offer_to_fill = offer_to_edit ? offer_to_edit : (last_offer ? last_offer : false);
            if(offer_to_fill){
                //$scope.offer = offer_to_fill;

                $scope.offer.is_standart      = offer_to_fill.is_standart;
                $scope.offer.expiration_time  = offer_to_fill.expiration_time;
                $scope.offer.location_city    = offer_to_fill.location_city;
                $scope.offer.location_country = offer_to_fill.location_country;

                var country = $filter('filter')($rootScope.countryList, {alpha2Code: $scope.offer.location_country});
                if(country.length){
                    country = country[0];
                    $scope.offer.initial_country = country.name;
                }

                gProxy.getDetails($scope.offer.location_city, function(data){
                    $timeout(function(){
                        $scope.offer.initial_city = data.name;
                    });
                });

                $scope.offer.amount_lui = $filter('gulden')(offer_to_fill.amount_lui, false);
                $scope.offer.amount_etc = offer_to_fill.amount_etc;
                $scope.offer.comment    = offer_to_fill.comment;
                $scope.offer.target     = offer_to_fill.target;

                if(offer_to_fill.payment_types.length){
                    $scope.offer.payment_types = offer_to_fill.payment_types.split(',');

                    angular.forEach($scope.offer.payment_types,function(value){
                        if(!market.paymentTypes.hasOwnProperty(value)){
                            $scope.offer.payment_type_other = value;
                        }
                    });  
                }
                

                var contacts = offer_to_fill.contacts.split(',');
                
                if(angular.isDefined(contacts[0])){
                    $scope.offer.contacts.email = contacts[0];
                }

                if(angular.isDefined(contacts[1])){
                    $scope.offer.contacts.phone = contacts[1];
                }

                $scope.recount('target');
            }

            $scope.addOffer = function(offer){
                if(!$scope.gOfferForm.$valid){
                    return;
                }

                var o = angular.copy(offer);
                
                o.fee = o.is_premium ? o.fee_premium : o.fee_standart;
                // o.location = o.location.country + ', ' + o.location.city;
                o.contacts = [o.contacts.email,o.contacts.phone].join(",");
                if(o.payment_type_other) o.payment_types.push(o.payment_type_other);
                o.payment_types = o.payment_types.join(",");
                //o.comment = o.comment + (o.comment?' ':'') + o.deal_details;
                o.target = o.currency;
                
                if($routeParams.offer_hash){
                    var safe = $filter('filter')($rootScope.safes, $routeParams.offer_hash);
                    if(safe.length){
                        backend.pushUpdateOffer(
                            safe[0].wallet_id, $routeParams.offer_hash, o.offer_type, o.amount_lui, o.target, o.location_city, o.location_country, o.contacts, 
                            o.comment, o.expiration_time, o.fee, o.amount_etc, o.payment_types, o.bonus, o.deal_details,
                            function(data){
                                informer.success('Спасибо. Заявка Обновлена');
                            }
                        );
                    }else{
                        informer.error('Не найден сейф, с которого было опубликовано предложение');
                    }
                }else{
                    backend.pushOffer(
                        o.wallet_id, o.offer_type, o.amount_lui, o.target, o.location_city, o.location_country, o.contacts, 
                        o.comment, o.expiration_time, o.fee, o.amount_etc, o.payment_types, o.bonus, o.deal_details,
                        function(data){
                            informer.success('Спасибо. Заявка Добавлена');
                        }
                    );
                }

                
            };
        }
    ]);

}).call(this);