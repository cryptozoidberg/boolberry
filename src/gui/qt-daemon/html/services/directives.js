(function() {
  'use strict';
  
  var module = angular.module('app.directives', []);

  module.directive('owlCarousel', ['$timeout', 'templateCompiler',function ($timeout, templateCompiler) {
    return {
      restrict: 'A',
      scope: {
        options : '=',
        items: '='
      },
      link: function (scope, element, attrs) {

        var options     = scope.options;
        var items       = scope.items;
        var template_id = $(element).attr('data-template-id');

        var buildHtml = function(items){
          var owl = $(element).data('owlCarousel');
          if(owl !== ''){
            console.log($(element).data('owlCarousel'));
            $(element).data('owlCarousel').destroy();
          }
          return $timeout(function(){
            var html = '';
            for (var i in items){
                html += templateCompiler.bind(template_id,items[i]);
            }
            return html;
          });
        }
              

        var init = function(){
          if(angular.isDefined(owl)){
            owl.destroy();
          }
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

        };

        var count = 0;
        if(angular.isDefined(scope.items)){
          count = angular.copy(items).length;

          scope.$watch(function(){
            if(count !== items.length){
              console.log('new element');
              count = items.length;
              buildHtml(items).then(function(data){
                $(element).html(data);
                init();
              });
            }
          });
        }
          
        if(count){
          $(element).html('');
          buildHtml(items).then(function(data){
            $(element).html(data);
            init();
          });
        }else{
          init();
        }

      }
    };
  }]);

  module.directive('owlCarouselItem', [function() {
      return {
          restrict: 'A',
          transclude: false,
          link: function(scope, element) {
            scope.addCarouselItem(element);
          }
      };
  }]);


  module.service('templateCompiler', function ($compile, $templateCache, $rootScope) {
    var service = {}
    var scope;
    
    this.bind = function (template_id, data) {
      scope = $rootScope.$new();
      angular.extend(scope, data);

      var template = $templateCache.get(template_id);
      if(template){
        var link = $compile(template);
        var content = link(scope);
        scope.$apply();
        return content.html();
      }else{
        return '';
      }
      
    };  
  });

  module.directive('datepickerPopup', function (){
    return {
        restrict: 'EAC',
        require: 'ngModel',
        link: function(scope, element, attr, controller) {
              //remove the default formatter from the input directive to prevent conflict
              console.log('picker');
              controller.$formatters.shift();
          }
        }
  });


}).call(this);