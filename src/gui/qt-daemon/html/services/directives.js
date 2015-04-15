(function() {
  'use strict';
  
  var module = angular.module('app.directives', []);

  module.directive('owlCarousel', function () {
    return {
        restrict: 'A',
        link: function (scope, element, attrs) {
            var options = scope.$eval($(element).attr('data-options'));

            var options = {
              items: 1,
              autoHeight: true,
              nav: false,
              dots: false,
              margin: 16,
              //callbacks: sliderInit();
              mouseDrag: false,
              touchDrag: false,
              callbacks: true,
              smartSpeed: 100,
              // URLhashListener: true,
              autoplayHoverPause: true,
              // startPosition: 'URLHash',
              // onInitialized : hideModalAdd
            };

            $(element).owlCarousel(options);
        }
    };
});


}).call(this);