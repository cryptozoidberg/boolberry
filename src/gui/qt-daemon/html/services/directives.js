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

  module.directive('disabledLink', [function() {
      return {
          restrict: 'A',
          link: function(scope, element, attrs) {
            scope.$watch(attrs.disabledLink,
              function(v){
                if(v){
                  $(element).removeClass('disabled-link');
                  $(element).click(function(event){
                    return true;
                  });
                }else{
                  $(element).addClass('disabled-link');
                  $(element).click(function(event){
                    event.preventDefault();
                  });
                }
              }
            );
          }
      };
  }]);



  module.service('templateCompiler', function ($compile, $templateCache, $rootScope) {
    var service = {};
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

  // module.directive('safemenu', [function() {
  //   return {
  //       restrict: 'E',
  //       scope: {
  //         safe: '=safe'
  //       },
  //       controller: 'safeMenu',
  //       templateUrl: 'views/safe-menu.html'
  //   };
  // }]);

  // module.controller('safeMenu', ['$scope','$rootScope','informer','$filter',
  //     function($scope,$rootScope,informer,$filter){
       
  //     $rootScope.closeWallet = function(wallet_id){
  //         backend.closeWallet(wallet_id, function(data){
  //             console.log(data);
  //             for (var i in $rootScope.safes){
  //                 if($rootScope.safes[i].wallet_id == wallet_id){
  //                     $rootScope.safes.splice(i,1);
  //                 }
  //             }
  //             backend.reloadCounters();
  //             backend.loadMyOffers();
  //             var path = $location.path();
              
  //             if(path.indexOf('/safe/') > -1){
  //                 $location.path('/safes');
  //             }
  //         });
  //     };

  //     $scope.startMining = function(wallet_id){
  //         backend.startPosMining(wallet_id);
  //         var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
  //         if(safe.length){
  //             safe[0].is_mining_set_manual = true; // flag to understand that we want to prevent auto mining
  //         }
  //     }

  //     $scope.stopMining = function(wallet_id){
  //         backend.stopPosMining(wallet_id);
  //         var safe = $filter('filter')($rootScope.safes,{wallet_id : wallet_id});
  //         if(safe.length){
  //             safe[0].is_mining_set_manual = true; // flag to understand that we want to prevent auto mining
  //         }
  //     }

  //     $scope.resynch = function(wallet_id){
  //         console.log('RESYNCH WALLET ::' + wallet_id);
  //         backend.resync_wallet(wallet_id, function(result){
  //             console.log(result);
  //         });
  //     };

  //     $scope.registerAlias = function(safe){ //TODO check safe data
          
  //         var modalInstance = $modal.open({
  //             templateUrl: "views/create_alias.html",
  //             controller: 'createAliasCtrl',
  //             size: 'md',
  //             windowClass: 'modal fade in',
  //             resolve: {
  //                 safe: function(){
  //                     return safe;
  //                 }
  //             }
  //         });
  //     };

  //     }
  // ]);

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

  module.directive('walletAddress', ['backend',function(backend) {
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