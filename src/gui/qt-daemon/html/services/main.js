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

    module.factory('loader',['$modal',function($modal){
        var ModalInstanceCtrl = function($scope, $modalInstance) {

            $scope.cancel = function() {
              $modalInstance.dismiss('cancel');
            };
        };


        return {
            open: function(message){
               
                if(!angular.isDefined(message)){
                    message = 'Пожалуйста, подождите...';
                }

                var modalHtml = '<div class="modal-header btn btn-primary btn-block">';
                modalHtml += '<button type="button" class="close" data-dismiss="modal" ng-click="cancel()">';
                modalHtml += '<span aria-hidden="true">.</span><span class="sr-only">Close</span>';
                modalHtml += '</button>';
                modalHtml += '<h4 class="modal-title text-center">Загружается...</h4>';
                modalHtml += '</div>';
                modalHtml += '<div class="modal-body">';
                modalHtml += '<span class="ifOnlineText loading text-primary">';
                modalHtml += '<i class="fa fa-2x fa-circle-o-notch fa-spin"></i> '+message;
                modalHtml += '</span>';
                modalHtml += '</div>';

                return $modal.open({
                  template: modalHtml,
                  controller: ModalInstanceCtrl,
                  backdrop: false,
                  size: 'sm'
                });
            },
            close: function(instance){
                if(angular.isDefined(instance)){
                    instance.close();
                }
            }
        }

    }]);

    module.factory('informer',['$modal',function($modal){
        var ModalInstanceCtrl = function($scope, $modalInstance) {

            $scope.cancel = function() {
              $modalInstance.dismiss('cancel');
            };
        };


        return {
            success: function(message){
                var type = 'success';
                var title = 'Выполнено';
                return this.open(message,type,title);
            },
            error: function(message){
                var type = 'danger';
                var title = 'Ошибка';
                return this.open(message,type,title);
            },
            warning: function(message){
                var type = 'warning';
                var title = 'Предупреждение';
                return this.open(message,type,title);
            },
            info: function(message){
                var type = 'info';
                var title = 'Информация';
                return this.open(message,type,title);
            },
            open: function(message,type,title){
               
                if(!angular.isDefined(message)){
                    message = '';
                }

                if(!angular.isDefined(type)){
                    type = 'info';
                }

                if(!angular.isDefined(title)){
                    title = 'Информация';
                }

                var modalHtml = '<div class="modal-header btn btn-'+type+' btn-block">';
                modalHtml += '<button type="button" class="close" data-dismiss="modal" ng-click="cancel()">';
                modalHtml += '<span aria-hidden="true">x</span><span class="sr-only">Close</span>';
                modalHtml += '</button>';
                modalHtml += '<h4 class="modal-title text-center">'+title+'</h4>';
                modalHtml += '</div>';
                modalHtml += '<div class="modal-body"><alert type="'+type+'">';
                modalHtml += message;
                modalHtml += '</alert><div>';
                modalHtml += '<button type="button" class="btn btn-'+type+' btn-block btn-sm" type="button" ng-click="cancel()">OK</button>';
                modalHtml += '</div>';
                modalHtml += '</div>';

                return $modal.open({
                  template: modalHtml,
                  controller: ModalInstanceCtrl,
                  backdrop: false,
                  size: 'sm'
                });
            },
            close: function(instance){
                if(angular.isDefined(instance)){
                    instance.close();
                }
            }
        }

    }]);

}).call(this);