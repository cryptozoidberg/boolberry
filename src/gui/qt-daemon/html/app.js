(function() {
  'use strict';
    var app = angular.module('app', [
        'ngRoute', 
        'ngSanitize', 
        'app.services',
        'app.backendServices',
        'app.dashboard',
        'app.navbar'
    ]);

    app.config(['$routeProvider', function($routeProvider) {
        $routeProvider
            .when('/', {
                templateUrl: 'views/index.html',
                controller: 'IndexController'
            })
            .when('/index', {
                templateUrl: 'views/index.html',
                controller: 'IndexController'
            })
            .when('/safes', {
                templateUrl: 'views/safes.html',
                controller: 'SafesController'
            })
            .when('/someRoute/:id', { // $routeParams object
                templateUrl: 'views/contact.html',
                controller: 'ParameterController'
            })
            .otherwise({
                templateUrl: 'views/routingError.html'
            });
        // TODO create directive for this
        // Turn all '*.html' links to '#/*'
        $(document).on('click', 'a[href*=".html"]', function() {
            var hash = $(this).attr('href').split("#")[1];
            var request = $(this).attr('href').replace('.html', '').split("#")[0];
            if (hash && hash != '') {
                location.hash = request + '/' + hash;
            } else {
                location.hash = request;
            }
            return false;
        });
    }]);

}).call(this);


//TODO move it somewhere!!!!
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