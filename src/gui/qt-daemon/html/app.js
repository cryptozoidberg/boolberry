(function() {
  'use strict';
    var app = angular.module('app', [
        'ui.bootstrap',
        'ui.chart',
        'angular-bootstrap-select',
        'ui.slider',
        "angucomplete-alt",
        "easypiechart",
        // 'google.places',

        'ngRoute', 
        'ngIdle',
        'ngSanitize',
        'validation.match', // allow to validate inputs match 
        'angularSlideables', // show/hide animations
        
        'app.app',
        'app.filters',
        'app.services',
        'app.backendServices',
        'app.dashboard',
        'app.directives',
        'app.safes',
        'app.guldens',
        'app.market' ,
        'app.settings',
        'app.contacts',
        'app.mining'

    ]);

    app.constant('CONFIG',{
        filemask : "*;;*.lui",
        CDDP : 8, //CURRENCY_DISPLAY_DECIMAL_POINT,
        standart_fee : 0.1,
        default_lang : 'ru'
    });

    app.config(['$routeProvider', 'IdleProvider', 'KeepaliveProvider', function($routeProvider, IdleProvider, KeepaliveProvider) {
        
        // configure Idle settings
        // IdleProvider.idle(0); // in seconds
        IdleProvider.timeout(10); // in seconds
        //KeepaliveProvider.interval(2); // in seconds

        var routes = [
            {route: '/',                           template: 'views/index.html'},
            {route: '/index',                      template: 'views/index.html'},
            {route: '/safes',                      template: 'views/safes.html'},
            {route: '/safe/:wallet_id',            template: 'views/safe.html'},
            {route: '/market',                     template: 'views/market.html'},
            {route: '/contacts',                   template: 'views/contacts.html'},
            {route: '/history',                    template: 'views/history.html'},
            {route: '/history/:contact_id',        template: 'views/history.html'},
            {route: '/deposits',                   template: 'views/deposits.html'},
            {route: '/settings',                   template: 'views/settings.html'},
            {route: '/sendG',                      template: 'views/sendG.html'},
            {route: '/sendG/:wallet_id',           template: 'views/sendG.html'},
            {route: '/sendGToContact/:address',    template: 'views/sendG.html'},

            {route: '/addGOfferBuy',               template: 'views/addGOffer.html'},
            {route: '/addGOfferBuy/:offer_hash',   template: 'views/addGOffer.html'},
            {route: '/addGOfferSell',              template: 'views/addGOffer.html'},
            {route: '/addGOfferSell/:offer_hash',  template: 'views/addGOffer.html'},

            {route: '/addOfferBuy',                template: 'views/addOffer.html'},
            {route: '/addOfferBuy/:offer_hash',    template: 'views/addOffer.html'},
            {route: '/addOfferSell',               template: 'views/addOffer.html'},
            {route: '/addOfferSell/:offer_hash',   template: 'views/addOffer.html'},

            {route: '/addContact',                   template: 'views/addContact.html'},
            {route: '/addContact/:address',          template: 'views/addContact.html'},
            {route: '/contact/:contact_id',          template: 'views/addContact.html'},
            {route: '/contact/:contact_id/:address', template: 'views/addContact.html'},
            
        ];

        for (var i in routes){
            $routeProvider.when(routes[i].route, {templateUrl: routes[i].template});
        }

        $routeProvider.otherwise({
            templateUrl: 'views/routingError.html'
        });
        
    }]);

    app.run(['$rootScope','Idle',function($rootScope, Idle){
        
        if(angular.isUndefined($rootScope.safes)){
            $rootScope.safes = [];
        }

        // Randomly shuffles strings. Mostly the emulator needs it
        String.prototype.shuffle = function () {
            var a = this.split(""),
                n = a.length;

            for(var i = n - 1; i > 0; i--) {
                var j = Math.floor(Math.random() * (i + 1));
                var tmp = a[i];
                a[i] = a[j];
                a[j] = tmp;
            }
            return a.join("");
        };

        // helper for pseudo-associative arrays in JS - works like .length but does it on objects
        Object.size = function(obj) {
            var size = 0, key;
            for (key in obj) {
                if (obj.hasOwnProperty(key)) size++;
            }
            return size;
        };
    }]);

}).call(this);
