(function() {
	'use strict';
	var module = angular.module('app.services', [])
    module.factory('utils', [function() {

        function Utils() {
            this.getRandomInt = function(min, max) {
                return Math.floor(Math.random() * (max - min + 1) + min);
            };
        }

        return new Utils();
    }]);

    module.service('gProxy',['$window','informer',
        function($window, informer){
        
            var targetFrameName = 'target';//have to include this iframe in your template body

            if(angular.isUndefined($window.frames[targetFrameName])){
                throw 'Cannot find iframe with name '+targetFrameName+' in DOM';
            }else{
                var iframe = $window.frames[targetFrameName];
            }

            var callbacks = {};
            var requestIndex = 0;

            $window.addEventListener('message', function(event){
                callbacks[event.data.requestIndex](event.data.data);
            });

            var oReturn =  {
                getPredictions: function(input, country, language, callback){
                    var message = {
                        method : 'get_predictions',
                        input : input,
                        country : country,
                        language : language
                    }

                    oReturn.sendMessage(message, callback);
                },
                sendMessage: function(message, callback){
                    requestIndex++;
                    message.requestIndex = requestIndex;
                    callbacks[message.requestIndex.toString()] = callback;
                    iframe.postMessage(message, "*");
                },
                getDetails: function(placeId, callback){
                    var message = {
                        method : 'get_details',
                        placeId : placeId
                    }

                    oReturn.sendMessage(message, callback);
                }
            };

            return oReturn;
        }
    ]);

    module.factory('loader',['$modal',function($modal){
        var ModalInstanceCtrl = function($scope, $modalInstance) {

            $scope.cancel = function() {
              $modalInstance.dismiss('cancel');
            };
        };


        return {
            open: function(message){
               
                if(!angular.isDefined(message)){
                    message = 'Пожалуйста, подождите...';
                }

                var modalHtml = '<div class="modal-header btn btn-primary btn-block">';
                modalHtml += '<button type="button" class="close" data-dismiss="modal" ng-click="cancel()">';
                modalHtml += '<span aria-hidden="true">.</span><span class="sr-only">Close</span>';
                modalHtml += '</button>';
                modalHtml += '<h4 class="modal-title text-center">Загружается...</h4>';
                modalHtml += '</div>';
                modalHtml += '<div class="modal-body">';
                modalHtml += '<div class="text-center margin-bottom">';
                modalHtml += '<span  class="text-primary"><i class="fa fa-3x fa-spinner fa-pulse"></i></span>';
                modalHtml += '</div>';
                modalHtml += '<div>';
                modalHtml += message;
                modalHtml += '</div>';
                modalHtml += '</div>';

                return $modal.open({
                  template: modalHtml,
                  controller: ModalInstanceCtrl,
                  backdrop: false,
                  size: 'sm'
                });
            },
            close: function(instance){
                if(angular.isDefined(instance)){
                    instance.close();
                }
            }
        }

    }]);

    module.factory('informer',['$modal',function($modal){
        var ModalInstanceCtrl = function($scope, $modalInstance) {

            $scope.cancel = function() {
              $modalInstance.dismiss('cancel');
            };
        };


        return {
            success: function(message){
                var type = 'success';
                var title = 'Выполнено';
                return this.open(message,type,title);
            },
            error: function(message){
                var type = 'danger';
                var title = 'Ошибка';
                return this.open(message,type,title);
            },
            warning: function(message){
                var type = 'warning';
                var title = 'Предупреждение';
                return this.open(message,type,title);
            },
            info: function(message){
                var type = 'info';
                var title = 'Информация';
                return this.open(message,type,title);
            },
            open: function(message,type,title){
               
                if(!angular.isDefined(message)){
                    message = '';
                }

                if(!angular.isDefined(type)){
                    type = 'info';
                }

                if(!angular.isDefined(title)){
                    title = 'Информация';
                }

                var modalHtml = '<div class="modal-header btn btn-'+type+' btn-block">';
                modalHtml += '<button type="button" class="close" data-dismiss="modal" ng-click="cancel()">';
                modalHtml += '<span aria-hidden="true">x</span><span class="sr-only">Close</span>';
                modalHtml += '</button>';
                modalHtml += '<h4 class="modal-title text-center">'+title+'</h4>';
                modalHtml += '</div>';
                modalHtml += '<div class="modal-body"><alert type="'+type+'">';
                modalHtml += message;
                modalHtml += '</alert><div>';
                modalHtml += '<button type="button" class="btn btn-'+type+' btn-block btn-sm" type="button" ng-click="cancel()">OK</button>';
                modalHtml += '</div>';
                modalHtml += '</div>';

                return $modal.open({
                  template: modalHtml,
                  controller: ModalInstanceCtrl,
                  backdrop: false,
                  size: 'sm'
                });
            },
            close: function(instance){
                if(angular.isDefined(instance)){
                    instance.close();
                }
            }
        }
    }]);

    module.factory('gPlace', [function(){

        return {
            getById: function(placeId,callback){
                var request = {
                  placeId: placeId
                };

                var map = new google.maps.Map(document.getElementById('mapCanvas')); // 

                var service = new google.maps.places.PlacesService(map);
                service.getDetails(request, callback);

            }
        };
    }]);

    module.factory('txHistory', ['$rootScope',function($rootScope) {
        var returnObject = {
            reloadHistory: function() {
                var temp = [];
                angular.forEach($rootScope.safes, function(safe){
                    if(angular.isDefined(safe.history)){
                        angular.forEach(safe.history, function(item){
                            item.wallet_id = angular.copy(safe.wallet_id);
                            temp.push(item);
                        });
                    }
                },true); 
                return temp;
            },
            contactHistory: function(contact) {
                // contact example
                // { 
                //     group_index: 0, 
                //     name: '', 
                //     email: '', 
                //     phone: '',
                //     messengers: [
                //         {name: 'skype', nickname: 'doblon'}
                //     ],
                //     location: '', 
                //     comment: '',
                //     addresses : [
                //         'HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ',
                //         'HhTZP7Sy4FoDR1kJHbFjzd5gSnUPdpHWHj7Gaaeqjt52KS23rHGa1sN73yZYPt77TkN8VVmHrT5qmBJQGzDLYJGjQpxGRid'
                //     ]
                // };

                var history = returnObject.reloadHistory();
                var temp = [];
                angular.forEach(history, function(item){
                    if(angular.isDefined(contact.addresses) && contact.addresses.indexOf(item.remote_address) >-1){
                        temp.push(item);
                    }
                });
                return temp;
            }
        }
        return returnObject;
    }]);

    module.factory('market', [function() {

        this.paymentTypes = {
            'EPS' : 'Электронные платежные системы',
            'BC'  : 'Банковская карта',
            'BT'  : 'Банковский перевод',
            'CSH' : 'Наличные'
        };

        this.currencies = [
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

        return this;
    }]);


    module.factory('uuid', [function(){
        this.generate = function(){
              var s4 = function() {
                return Math.floor((1 + Math.random()) * 0x10000)
                  .toString(16)
                  .substring(1);
              }
              return s4() + s4() + '-' + s4() + '-' + s4() + '-' + s4() + '-' + s4() + s4() + s4();
        }
        return this;
    }]);


    module.factory('contacts', ['backend', '$rootScope',function(backend, $rootScope) { // work with $rootScope.settings.contacts

        if(angular.isUndefined($rootScope.settings.contacts)){
            $rootScope.settings.contacts = [];
        }

        

        var instance = { // contact instance structure by default
            group_index: 0, //required
            name: '', //required
            email: '', //required
            phone: '',
            messengers: [
                // ex. {name: 'skype', nickname: 'doblon'}
            ],
            location: '', // ???
            comment: '',
            addresses : [
                // 'HcTjqL7yLMuFEieHCJ4buWf3GdAtLkkYjbDFRB4BiWquFYhA39Ccigg76VqGXnsXYMZqiWds6C6D8hF5qycNttjMMBVo8jJ',
                // 'HhTZP7Sy4FoDR1kJHbFjzd5gSnUPdpHWHj7Gaaeqjt52KS23rHGa1sN73yZYPt77TkN8VVmHrT5qmBJQGzDLYJGjQpxGRid'
            ]
        };

        angular.extend(this, instance);

        instance.create = function(group_index, name, email){
            if(angular.isUndefined(group_index) || angular.isUndefined(name) || angular.isUndefined(email)){
                console.log('Error create contact :: group, name and email is a minimum set of data to create contact instance');
                return false;
            }
            var contact = angular.copy(contactInstance);
            contact.group_index = group_index;
            contact.name = name;
            contact.email = email;
        }

        contact.setGroup();
        contact.addAddress('dgfghfhdfb')

        return this;
    }]);

}).call(this);