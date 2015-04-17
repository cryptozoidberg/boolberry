(function() {
  'use strict';
  
  var module = angular.module('app.directives', []);

  module.directive('owlCarousel', [function () {
    return {
        restrict: 'A',
        link: function (scope, element, attrs) {
            var options = scope.$eval($(element).attr('data-options'));
            
            $(element).owlCarousel(options);
            var owl = $(element).data('owlCarousel');

            $(element).find('.owl-prev').each(function(num,item){
              $(item).click(function(){
                owl.prev();
              });
            });

            $(element).find('.owl-next').each(function(num,item){
              $(item).click(function(){
                owl.next();
              });
            });

        }
    };
  }]);


  module.directive('turnLinks', [function(){
    return {
        restrict: 'A',
        link: function (scope, element, attrs) {
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
        }
    };
  }]);


}).call(this);