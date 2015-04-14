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

}).call(this);