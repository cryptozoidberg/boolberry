(function() {
  'use strict';
    var app = angular.module('app', [
        'ui.bootstrap',
        'ui.chart',
        'angular-bootstrap-select',

        'ngRoute', 
        'ngSanitize',
        'validation.match', // allow to validate inputs match 
        'angularSlideables', // show/hide animations

        'app.filters',
        'app.services',
        'app.backendServices',
        'app.dashboard',
        'app.navbar',
        'app.directives',
        'app.safes',
        'app.guldens',

    ]);

    app.config(['$routeProvider', function($routeProvider) {
        
        var routes = [
            {route: '/',                 template: 'views/index.html'},
            {route: '/index',            template: 'views/index.html'},
            {route: '/safes',            template: 'views/safes.html'},
            {route: '/safe/:wallet_id',  template: 'views/safe.html'},
            {route: '/market',           template: 'views/market.html'},
            {route: '/contacts',         template: 'views/contacts.html'},
            {route: '/history',          template: 'views/history.html'},
            {route: '/deposits',         template: 'views/deposits.html'},
            {route: '/settings',         template: 'views/settings.html'},
            {route: '/safety',           template: 'views/safety.html'},
            {route: '/maining',          template: 'views/maining.html'},
            {route: '/interface',        template: 'views/interface.html'},
            {route: '/common',           template: 'views/common.html'},
            {route: '/sendG',            template: 'views/sendG.html'},
            {route: '/buyG',             template: 'views/buyG.html'},
            {route: '/sellG',            template: 'views/sellG.html'},
            {route: '/buyProduct',       template: 'views/buyProduct.html'},
            {route: '/sellProduct',      template: 'views/sellProduct.html'},
        ];

        for (var i in routes){
            $routeProvider.when(routes[i].route, {templateUrl: routes[i].template});
        }

        $routeProvider.otherwise({
            templateUrl: 'views/routingError.html'
        });
        
    }]);

    app.run([function(){
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


//TODO move it somewhere!!!!
