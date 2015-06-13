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

          scope.$watch('items',function(){
            //if(count !== items.length){ //new element
              count = items.length;
              buildHtml(items).then(function(data){
                $(element).html(data);
                $timeout(function(){
                  init();
                },500);
                
              });
            //}
            console.log('items changed');
            console.log(items);
            
          },true);
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
              controller.$formatters.shift();
          }
        }
  });

  module.directive('wallet_address', ['backend',function() {
    return {
      require: 'ngModel',
      link: function(scope, elm, attrs, ctrl) {
        ctrl.$validators.wallet_address = function(modelValue, viewValue) {
          if (ctrl.$isEmpty(modelValue)) {
            return false;
          }

          return backend.validateAddress(viewValue);
        };
      }
    };
  }]);

  // module.directive('selectText', ['$window',function ($window){
  //   return {
  //     restrict: 'A',
  //     link: function(scope, element, attr, controller) {
  //       var selectElement;

  //       if ($window.document.selection) {
  //         selectElement = function(element) {
  //           var range = $window.document.body.createTextRange();
  //           range.moveToElementText(element[0]);
  //           range.select();
  //         };
  //       } else if ($window.getSelection) {
  //         selectElement = function(element) {
  //           var range = $window.document.createRange();
  //           range.selectNode(element[0]);
  //           $window.getSelection().addRange(range);
  //         };
  //       }

  //       selectElement(element);
  //     }
  //   }
  // }]);

  // module.directive('currencyParser', function (){
  //   return {
  //       restrict: 'A',
  //       link: function(scope, element, attr) {
  //             //remove the default formatter from the input directive to prevent conflict
  //             //controller.$formatters.shift();
  //             var currencies = [];
  //             element.children().each(function(){
  //               var item = {
  //                 code: $(this).attr('data-value'),
  //                 title: $(this).html()
  //               };
  //               currencies.push(item);
                
  //             });
  //             console.log(JSON.stringify(currencies));
  //         }
  //       }
  // });


}).call(this);