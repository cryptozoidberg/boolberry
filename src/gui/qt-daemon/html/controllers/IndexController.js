(function() {
    'use strict';
    var module = angular.module('app.dashboard',[]);

    module.controller('PageController', ['backend', '$scope', function(backend, $scope) {
        $scope.backend = backend;
    }]);

    module.controller('IndexController', ['utils', 'backend', '$scope', function(utils, backend, $scope) {
        $scope.settings = {
            maxWidgets: 12,
            userSettings: {
                'sendG_showPassword': true
            }
        };

        init();

        backend.openWallet('/home/master/Lui/test_wallet.lui', '12345',function(data){
            console.log(data);
        });

        $scope.openFiledialog = function(){
            var caption = "Please, choose the file";
            var filemask = "*.lui";
            backend.openFiledialog(caption, filemask);
        };

        function init() {
            // setUpWidgets();
            
        }

        function activate() {
            console.log('index controller activated');
        }

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
                return ($scope.getWidgetCount >= $scope.settings.maxWidgets);
            };
            $scope.changeWidget = function(id, type) {
                var col = ($scope.widgetColumns.left[id]) ? $scope.widgetColumns.left : $scope.widgetColumns.right;

                col[id].type = type;
                col[id].name = $scope.getWidgetNameByType(type);

                // Patches
                if (type == 'activeMining') {
                    doPlot("right"); // draw charts
                }
                if (type == 'backendInfo') {
                    //console.log($scope.backend);
                }
            };
            $scope.widgets = {
                '': {name: 'Выберите тип виджета справа', link: ''},
                'ingoingPayments': {name: 'Входящие платежи', link: ''},
                'outgoingPayments': {name: 'Исходящие платежи', link: ''},
                'activeMining': {name: 'Активная добыча', link: ''},
                'coinDemand': {name: 'Спрос на гульдены в системе', link: ''},
                'coinOffer': {name: 'Предложение гульденов в системе', link: ''},
                'commodityDemand': {name: 'Спрос на товар', link: ''},
                'commodityOffer': {name: 'Предложения товаров', link: ''},
                'currencyFavorite': {name: 'Избранное – Валюта', link: ''},
                'commodityFavorite': {name: 'Избранное – Товар', link: ''},
                'lastContacts': {name: 'Последние контакты', link: ''},
                'exchangeDeals': {name: 'Сделки на биржах', link: ''},
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
    }]);
}).call(this);