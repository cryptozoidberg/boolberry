(function() {
    'use strict';
    var module = angular.module('app.dashboard',[]);



    module.controller('indexController', ['utils', 'backend', '$scope', '$modal','$timeout','emulator','$rootScope', 'informer',
        function(utils, backend, $scope, $modal, $timeout, emulator, $rootScope, informer) {
            $scope.settings = {
                maxWidgets: 12,
                userSettings: {
                    'sendG_showPassword': true
                }
            };

            $scope.safes_owl_options  = {
              items: 2,
              navText: '',
              responsive: false
            };
            
            setUpWidgets();

            function setUpWidgets() {
                $scope.widgetColumns = {
                    'left': {},
                    'right': {}
                };

                $scope.addWidget = function(col) {
                    var widget_number = $scope.getWidgetCount() + '_' + utils.getRandomInt(0, 1000000); // getWidgetCount to avoid auto-sorting by angular
                    var widget = { id: widget_number, type: '', name: $scope.getWidgetNameByType('') };
                    var col = (col=='left') ? $scope.widgetColumns.left : $scope.widgetColumns.right;
                    col[widget_number] = widget;
                };
                $scope.removeWidget = function(id) {
                    delete $scope.widgetColumns.left[id];
                    delete $scope.widgetColumns.right[id];
                };
                $scope.getWidgetCount = function() {
                    return Object.size($scope.widgetColumns.left) + Object.size($scope.widgetColumns.right);
                };
                $scope.widgetCountSanityCheck = function() {
                    return ($scope.getWidgetCount() >= $scope.settings.maxWidgets);
                };

                $scope.am = {}; //active mining chart data

                $scope.changeWidget = function(id, type) {
                    var col = ($scope.widgetColumns.left[id]) ? $scope.widgetColumns.left : $scope.widgetColumns.right;

                    col[id].type = type;
                    col[id].name = $scope.getWidgetNameByType(type);

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
                    // 'ingoingPayments': {name: 'Входящие платежи', link: ''},
                    // 'outgoingPayments': {name: 'Исходящие платежи', link: ''},
                    'activeMining': {name: 'Активная добыча', link: ''},
                    // 'coinDemand': {name: 'Спрос на гульдены в системе', link: ''},
                    // 'coinOffer': {name: 'Предложение гульденов в системе', link: ''},
                    // 'commodityDemand': {name: 'Спрос на товар', link: ''},
                    // 'commodityOffer': {name: 'Предложения товаров', link: ''},
                    // 'currencyFavorite': {name: 'Избранное – Валюта', link: ''},
                    // 'commodityFavorite': {name: 'Избранное – Товар', link: ''},
                    // 'lastContacts': {name: 'Последние контакты', link: ''},
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
            }
        }
    ]);
}).call(this);