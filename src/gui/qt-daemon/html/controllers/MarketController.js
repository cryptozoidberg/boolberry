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

            console.log($location.path());

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
    module.controller('addGOfferCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','$timeout',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location,$timeout){
            $scope.intervals = [1,3,5,14];

            console.log($location.path());

            $scope.currencies = [
                {"code":"AUD","title":"Австралийский доллар (AUD)"},
                {"code":"AZN","title":"Азербайджанский манат (AZN)"},
                {"code":"ALL","title":"Албанский лек (ALL)"},
                {"code":"DZD","title":"Алжирский динар (DZD)"},
                {"code":"AOA","title":"Ангольская кванза (AOA)"},
                {"code":"ARS","title":"Аргентинское песо (ARS)"},
                {"code":"AMD","title":"Армянский драм (AMD)"},
                {"code":"AWG","title":"Арубанский гульден (AWG)"},
                {"code":"AFA","title":"Афгани (AFA)"},
                {"code":"BSD","title":"Багамский доллар (BSD)"},
                {"code":"BDT","title":"Бангладешская така (BDT)"},
                {"code":"BBD","title":"Барбадосский доллар (BBD)"},
                {"code":"BHD","title":"Бахрейнский динар (BHD)"},
                {"code":"BZD","title":"Белизский доллар (BZD)"},
                {"code":"BYB","title":"Белорусский рубль (BYB)"},
                {"code":"BGL","title":"Болгарский лев (BGL)"},
                {"code":"BOB","title":"Боливиано (BOB)"},
                {"code":"BWP","title":"Ботсванская пула (BWP)"},
                {"code":"BRR","title":"Бразильский реал (BRR)"},
                {"code":"BND","title":"Брунейский доллар (ринггит) (BND)"},
                {"code":"BMD","title":"Бурмудский доллар (BMD)"},
                {"code":"BIF","title":"Бурундийский франк (BIF)"},
                {"code":"VUV","title":"Вату (VUV)"},
                {"code":"HUF","title":"Венгерский форинт (HUF)"},
                {"code":"VEB","title":"Венесуэльский боливар (VEB)"},
                {"code":"XCD","title":"Восточнокарибский доллар (XCD)"},
                {"code":"VND","title":"Вьетнамский донг (VND)"},
                {"code":"HTG","title":"Гаитянский гурд (HTG)"},
                {"code":"GMD","title":"Гамбийский даласи (GMD)"},
                {"code":"GHC","title":"Ганский седи (GHC)"},
                {"code":"GTQ","title":"Гватемальский кетсал (GTQ)"},
                {"code":"GNF","title":"Гвинейский франк (GNF)"},
                {"code":"GIP","title":"Гибралтарский фунт (GIP)"},
                {"code":"HML","title":"Гондурасская лемпира (HML)"},
                {"code":"GEL","title":"Грузинский лари (GEL)"},
                {"code":"ANG","title":"Гульден антильский (ANG)"},
                {"code":"DKK","title":"Датская крона (DKK)"},
                {"code":"RSD","title":"Динар (RSD)"},
                {"code":"AED","title":"Дирхам ОАЭ (AED)"},
                {"code":"STD","title":"Добра (STD)"},
                {"code":"ZWD","title":"Доллар Зимбабве (ZWD)"},
                {"code":"KYD","title":"Доллар Каймановых о-ов (KYD)"},
                {"code":"MYR","title":"Доллар малайзийский (ринггит) (MYR)"},
                {"code":"SBD","title":"Доллар Соломоновых о-ов (SBD)"},
                {"code":"USD","title":"Доллар США (USD)"},
                {"code":"TTD","title":"Доллар Тринидада и Тобаго (TTD)"},
                {"code":"FJD","title":"Доллар Фиджи (FJD)"},
                {"code":"DOP","title":"Доминиканское песо (DOP)"},
                {"code":"EUR","title":"Евро (EUR)"},
                {"code":"EGP","title":"Египетский фунт (EGP)"},
                {"code":"ZMK","title":"Замбийская квача (ZMK)"},
                {"code":"NIO","title":"Золотая кордоба (NIO)"},
                {"code":"ILS","title":"Израильский шекель (ILS)"},
                {"code":"INR","title":"Индийская рупия (INR)"},
                {"code":"IDR","title":"Индонезийская рупия (IDR)"},
                {"code":"JOD","title":"Иорданский доллар (JOD)"},
                {"code":"IQD","title":"Иракский динар (IQD)"},
                {"code":"IRR","title":"Иранский риал (IRR)"},
                {"code":"IEP","title":"Ирландский фунт (IEP)"},
                {"code":"ISK","title":"Исландская крона (ISK)"},
                {"code":"YER","title":"Йеменский риал (YER)"},
                {"code":"KHR","title":"Камбоджский риель (KHR)"},
                {"code":"CAD","title":"Канадский доллар (CAD)"},
                {"code":"QAR","title":"Катарский риал (QAR)"},
                {"code":"KES","title":"Кенийский шиллинг (KES)"},
                {"code":"PGK","title":"Кина (PGK)"},
                {"code":"CYP","title":"Кипрский фунт (CYP)"},
                {"code":"CNY","title":"Китайский юань (CNY)"},
                {"code":"COP","title":"Колумбийское песо (COP)"},
                {"code":"KMF","title":"Коморский франк (KMF)"},
                {"code":"BAM","title":"Конвертируемая марка (BAM)"},
                {"code":"CRC","title":"Костариканский колон (CRC)"},
                {"code":"CUP","title":"Кубинское песо (CUP)"},
                {"code":"KWD","title":"Кувейтский динар (KWD)"},
                {"code":"HRK","title":"Куна (HRK)"},
                {"code":"KGS","title":"Кыргызский сом (KGS)"},
                {"code":"MMK","title":"Кьят (MMK)"},
                {"code":"LAK","title":"Лаосский кип (LAK)"},
                {"code":"LVL","title":"Латвийский лат (LVL)"},
                {"code":"SLL","title":"Леоне (SLL)"},
                {"code":"LRD","title":"Либерийский доллар (LRD)"},
                {"code":"LBP","title":"Ливанский фунт (LBP)"},
                {"code":"LYD","title":"Ливийский динар (LYD)"},
                {"code":"LTL","title":"Литовский лит (LTL)"},
                {"code":"LSL","title":"Лоти (LSL)"},
                {"code":"MUR","title":"Маврикийская рупия (MUR)"},
                {"code":"MRO","title":"Мавританская угийя (MRO)"},
                {"code":"MKD","title":"Македонский денар (MKD)"},
                {"code":"MWK","title":"Малавийская квача (MWK)"},
                {"code":"MGF","title":"Малагасийский франк (MGF)"},
                {"code":"MVR","title":"Мальдивская руфия (MVR)"},
                {"code":"MTL","title":"Мальтийская лира (MTL)"},
                {"code":"MAD","title":"Марокканский дирхам (MAD)"},
                {"code":"MXN","title":"Мексиканское песо (MXN)"},
                {"code":"MZM","title":"Мозамбикский метикал (MZM)"},
                {"code":"MDL","title":"Молдавский лей (MDL)"},
                {"code":"MNT","title":"Монгольский тугрик (MNT)"},
                {"code":"ERN","title":"Накфа (ERN)"},
                {"code":"BTN","title":"Нгултрум (BTN)"},
                {"code":"NPR","title":"Непальская рупия (NPR)"},
                {"code":"NGN","title":"Нигерийская найра (NGN)"},
                {"code":"NZD","title":"Новозеландский доллар (NZD)"},
                {"code":"PEN","title":"Новый соль (PEN)"},
                {"code":"TWD","title":"Новый тайваньский доллар (TWD)"},
                {"code":"NOK","title":"Норвежская крона (NOK)"},
                {"code":"OMR","title":"Оманский риал (OMR)"},
                {"code":"TOP","title":"Паанга (TOP)"},
                {"code":"PKR","title":"Пакистанская рупия (PKR)"},
                {"code":"PYG","title":"Парагвайский гуарани (PYG)"},
                {"code":"PLZ","title":"Польский злотый (PLZ)"},
                {"code":"RUR","title":"Российский рубль (RUR)"},
                {"code":"ROL","title":"Румынский лей (ROL)"},
                {"code":"SVC","title":"Сальвадорский колон (SVC)"},
                {"code":"WST","title":"Самоанская тала (WST)"},
                {"code":"SAR","title":"Саудовский риал (SAR)"},
                {"code":"SZL","title":"Свазилендская лилангени (SZL)"},
                {"code":"KPW","title":"Северокорейская вона (KPW)"},
                {"code":"SCR","title":"Сейшельская рупия (SCR)"},
                {"code":"SGD","title":"Сингапурский доллар (SGD)"},
                {"code":"SYP","title":"Сирийский фунт (SYP)"},
                {"code":"SOS","title":"Сомалийский шиллинг (SOS)"},
                {"code":"SDD","title":"Суданский динар (SDD)"},
                {"code":"SRG","title":"Суринамский гульден (SRG)"},
                {"code":"THB","title":"Таиландский бат (THB)"},
                {"code":"TZS","title":"Танзанский шиллинг (TZS)"},
                {"code":"KZT","title":"Тенге (KZT)"},
                {"code":"SIT","title":"Толар (SIT)"},
                {"code":"TND","title":"Тунисский динар (TND)"},
                {"code":"TRY","title":"Турецкая лира (TRY)"},
                {"code":"TMM","title":"Туркменский манат (TMM)"},
                {"code":"UGS","title":"Угандийский шиллинг (UGS)"},
                {"code":"UZS","title":"Узбекский сум (UZS)"},
                {"code":"UAH","title":"Украинская гривна (UAH)"},
                {"code":"UYP","title":"Уругвайское песо (UYP)"},
                {"code":"PHP","title":"Филиппинское песо (PHP)"},
                {"code":"DJF","title":"Франк Джибути (DJF)"},
                {"code":"XOF","title":"Франк КФА (XOF)"},
                {"code":"XAF","title":"Франк КФА (XAF)"},
                {"code":"XOP","title":"Франк КФА (XOP)"},
                {"code":"XPF","title":"Франк КФП (XPF)"},
                {"code":"RWF","title":"Франк Руанды (RWF)"},
                {"code":"GBP","title":"Фунт стерлингов (GBP)"},
                {"code":"FKP","title":"Фунт Фолклендских о-ов (FKP)"},
                {"code":"CZK","title":"Чешская крона (CZK)"},
                {"code":"CLP","title":"Чилийское песо (CLP)"},
                {"code":"SEK","title":"Шведская крона (SEK)"},
                {"code":"CHF","title":"Швейцарский франк (CHF)"},
                {"code":"LKR","title":"Шри-Ланкийская рупия (LKR)"},
                {"code":"ESC","title":"Эквадорский сукре (ESC)"},
                {"code":"CVE","title":"Эскудо Кабо-Верд (CVE)"},
                {"code":"EEK","title":"Эстонская крона (EEK)"},
                {"code":"ETB","title":"Эфиопский быр (ETB)"},
                {"code":"ZAR","title":"Южноафриканский рэнд (ZAR)"},
                {"code":"KRW","title":"Южнокорейская вона (KRW)"},
                {"code":"JMD","title":"Ямайский доллар (JMD)"},
                {"code":"JPY","title":"Японская иена (JPY)"},
                {"code":"BTC","title":"Биткоин (BTC)"}
            ];

            $scope.offer_types = [
                {key : 1, value: 'Покупка гульденов'},
                {key : 0, value: 'Продажа гульденов'}
            ];

            $scope.payment_types = {
                'EPS' : 'Электронные платежные системы',
                'BC'  : 'Банковская карта',
                'BT'  : 'Банковский перевод',
                'CSH' : 'Наличные'
            };

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
                payment_types: []
            };

            if($location.path() == '/addGOfferSell'){
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

                o.payment_types = o.payment_types.join(",");
                informer.info(JSON.stringify(o));

                //o.offer_type = 1; //TODO
                
                // 
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