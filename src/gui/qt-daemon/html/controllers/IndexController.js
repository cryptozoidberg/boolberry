(function() {
    'use strict';
    var module = angular.module('app.dashboard',[]);



    module.controller('indexController', ['utils', 'backend', '$scope', '$modal','$timeout','emulator','$rootScope', 'informer', 'txHistory', '$filter',
        function(utils, backend, $scope, $modal, $timeout, emulator, $rootScope, informer, txHistory, $filter) {
            $scope.db_settings = {
                maxWidgets: 12,
                userSettings: {
                    'sendG_showPassword': true
                }
            };

            $scope.pieStates = {
                danger: '#D9534F',
                warning: '#F0AD4E',
                info: '#5BC0DE',
                success: '#5CB85C',
            };

            $scope.pieChartoptions = {
                barColor: $scope.pieStates.info, // #5BC0DE info,  #F0AD4E warning,  danger
                trackColor: '#f2f2f2',
                // scaleColor: '#f2f2f2',
                scaleLength: 5,
                lineCap: 'butt',
                lineWidth: 9,
                size: 32,
                rotate: 0,
                animate: {},
                // easing: {}
            };

            $scope.safes_owl_options  = {
              items: 2,
              navText: '',
              responsive: false
            };
            
            setUpWidgets();

            $scope.row = 'wallet_id'; //sort by default

            $scope.order = function(key){
                $scope.row = key;
                $rootScope.safes = $filter('orderBy')($rootScope.safes,key);
            };

            function setUpWidgets() {
                

                if(angular.isUndefined($rootScope.settings.widgetColumns)){
                    $rootScope.settings.widgetColumns = {
                        'left': {},
                        'right': {}
                    };    
                }
                

                $scope.addWidget = function(col) {
                    var widget_number = $scope.getWidgetCount() + '_' + utils.getRandomInt(0, 1000000); // getWidgetCount to avoid auto-sorting by angular
                    var widget = { id: widget_number, type: '', name: $scope.getWidgetNameByType('') };
                    var col = (col=='left') ? $rootScope.settings.widgetColumns.left : $rootScope.settings.widgetColumns.right;
                    col[widget_number] = widget;
                };
                $scope.removeWidget = function(id) {
                    delete $rootScope.settings.widgetColumns.left[id];
                    delete $rootScope.settings.widgetColumns.right[id];
                };
                $scope.getWidgetCount = function() {
                    return Object.size($rootScope.settings.widgetColumns.left) + Object.size($rootScope.settings.widgetColumns.right);
                };
                $scope.widgetCountSanityCheck = function() {
                    return ($scope.getWidgetCount() >= $scope.db_settings.maxWidgets);
                };

                $scope.am = {}; //active mining chart data

                $scope.changeWidget = function(id, type) {
                    var col = ($rootScope.settings.widgetColumns.left[id]) ? $rootScope.settings.widgetColumns.left : $rootScope.settings.widgetColumns.right;

                    col[id].type = type;
                    col[id].name = $scope.getWidgetNameByType(type);
                    col[id].link = $scope.getWidgetLinkByType(type);

                    // Patches
                    if (type == 'activeMining') {

                        var euroFormatter = function (v, axis) {
                            return v.toFixed(axis.tickDecimals) + "G";
                        }

                        $scope.am.data = [{
                            data: emulator.getAMData(),
                            label: "Safelabel1"
                        }, {
                            data: emulator.getAMData(),
                            label: "Safelabel2",
                            // yaxis: 2
                        }];
                        var position = "right";
                        $scope.am.options = {
                            xaxes: [{
                                mode: 'time'
                            }],
                            yaxes: [{
                                min: 0
                            }, {
                                // align if we are to the right
                                alignTicksWithAxis: position == "right" ? 1 : null,
                                position: position,
                                tickFormatter: euroFormatter
                            }],
                            legend: {
                                position: 'sw'
                            },
                            colors: ["#1ab394"],
                            grid: {
                                color: "#999999",
                                clickable: true,
                                tickColor: "#D4D4D4",
                                borderWidth:0,
                                hoverable: true //IMPORTANT! this is needed for tooltip to work,

                            },
                            tooltip: true,
                            tooltipOpts: {
                                content: "%s for %x was %y",
                                xDateFormat: "%y-%0m-%0d",

                                onHover: function(flotItem, $tooltipEl) {
                                    // console.log(flotItem, $tooltipEl);
                                }
                            }
                        };
                    }
                    if (type == 'backendInfo') {
                        //console.log($scope.backend);
                    }
                };
                $scope.widgets = {
                    '': {name: 'Выберите тип виджета справа', link: ''},
                    'ingoingPayments': {name: 'Входящие платежи', link: 'history'},
                    'outgoingPayments': {name: 'Исходящие платежи', link: 'history'},
                    'activeMining': {name: 'Активная добыча', link: 'deposits'},
                    'coinDemand': {name: 'Спрос на гульдены в системе', link: 'market'},
                    'coinOffer': {name: 'Предложение гульденов в системе', link: 'market'},
                    'commodityDemand': {name: 'Спрос на товар', link: 'market'},
                    'commodityOffer': {name: 'Предложения товаров', link: 'market'},
                    'currencyFavorite': {name: 'Избранное – Валюта', link: 'market'},
                    'commodityFavorite': {name: 'Избранное – Товар', link: 'market'},
                    'lastContacts': {name: 'Последние контакты', link: 'contacts'},
                    // 'exchangeDeals': {name: 'Сделки на биржах', link: ''},
                    'backendInfo': {name: 'Информация о сети', link: 'settings'}
                };

                $scope.widgetOrder = Object.keys($scope.widgets);

                $scope.getWidgetNameByType = function(type) {
                    if ($scope.widgets.hasOwnProperty(type)) {
                        return $scope.widgets[type].name;
                    } else {
                        return 'Виджет '+type;
                    }
                };

                $scope.getWidgetLinkByType = function(type) {
                    if ($scope.widgets.hasOwnProperty(type)) {
                        return $scope.widgets[type].link;
                    } else {
                        return '';
                    }
                };

                $scope.fav_currency = function(item){
                    if((item.offer_type == 2 || item.offer_type == 3) && $rootScope.settings.system.fav_offers_hash.indexOf(item.tx_hash) > -1){
                        return true;
                    }
                    // return true;
                }

                $scope.history = [];

                $scope.$watch(
                    function(){
                        return $rootScope.safes;
                    },
                    function(){
                        $scope.history = txHistory.reloadHistory();
                    },
                    true
                );

                

                $scope.fav_goods = function(item){
                    if((item.offer_type == 0 || item.offer_type == 1) && $rootScope.settings.system.fav_offers_hash.indexOf(item.tx_hash) > -1){
                        return true;
                    }
                }
            }
        }
    ]);
}).call(this);