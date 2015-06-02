(function() {
    'use strict';
    var module = angular.module('app.contacts',[]);

    module.controller('contactGroupsCtrl', ['$scope','backend', '$modalInstance','informer','$rootScope', '$timeout', 'uuid',
        function($scope, backend, $modalInstance, informer, $rootScope, $timeout, uuid) {
            
            $scope.new_group = {
                name: ''
            };

            $scope.collapse = {
                new_group: true,
                warning:   true
            };


            if(angular.isUndefined($rootScope.settings.contacts)){
                $rootScope.settings.contacts = [];
            }

            if(angular.isUndefined($rootScope.settings.contact_groups)){
                $rootScope.settings.contact_groups = [ 
                    {id: uuid.generate(), name: 'Проверенные'},
                    {id: uuid.generate(), name: 'Черный список'},
                    {id: uuid.generate(), name: 'Магазины'},
                ];
            }

            $scope.items_collapse = [];

            $scope.close = function(){
                $modalInstance.close();
            }

            $scope.addGroup = function(group){
                group.id = uuid.generate();
                $rootScope.settings.contact_groups.push(angular.copy(group));
                group.name = '';
                $scope.collapse.new_group = true;
            }

            $scope.removeGroup = function(group){
                $scope.collapse.warning = false;
                $scope.group_to_delete = group;
            }

            $scope.realRemoveGroup = function(group){
                angular.forEach($rootScope.settings.contact_groups, function(item, index){
                    // alert(JSON.stringify(item.id)+' ----->>>>> '+JSON.stringify(group.id));
                    if(item.id == group.id){
                        $rootScope.settings.contact_groups.splice(index,1);
                    }
                });
                $scope.collapse.warning = true;
            }
        }
    ]);

    module.controller('contactsCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','$modal',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location,$modal){
            $scope.contactGroups = function(){
                $modal.open({
                    templateUrl: "views/contact_groups.html",
                    controller: 'contactGroupsCtrl',
                    size: 'md',
                    windowClass: 'modal fade in',
                    // backdrop: false,
                    // resolve: {
                    //     tr: function(){
                    //         return tr;
                    //     }
                    // }

                });
            }

            $scope.getContactGroups = function(contact){
                var groups = [];
                angular.forEach(contact.group_ids,function(group_id){
                    var group = $filter('filter')($rootScope.settings.contact_groups, {id:group_id});
                    if(group.length){
                        groups.push(group[0].name);
                    }
                });
                return groups.join(', ');
            }
        }
    ]);

    module.controller('addContactCtrl',['backend','$rootScope','$scope','informer', '$location',
        function(backend, $rootScope, $scope, informer, $location){
            $scope.contact = {
                name: '',
                email: '',
                phone: '',
                group_ids: [],
                location: {
                    country: '',
                    city: ''
                },
                comment: '',
                ims: [],
                adresses: []
            };

            $scope.im = {
                name: '',
                nickname: ''
            };

            $scope.addIM = function(im){
                if(im.name.length && im.nickname.length){
                    $scope.contact.ims.push(im);
                    $scope.collapse.add_im = false;
                    $scope.im = {
                        name: '',
                        nickname: ''
                    };
                }
            }

            $scope.addContact = function(contact){
                $rootScope.settings.contacts.push(contact);
                $location.path('/contacts');
            }

            $scope.addAddress = function(address){
                $rootScope.settings.contacts.push(address);

            }
        }
    ]);



}).call(this);