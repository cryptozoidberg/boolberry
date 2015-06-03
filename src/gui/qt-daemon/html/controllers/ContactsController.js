(function() {
    'use strict';
    var module = angular.module('app.contacts',[]);

    module.controller('contactGroupsCtrl', ['$scope','backend', '$modalInstance','informer','$rootScope', '$timeout', 'uuid', '$filter',
        function($scope, backend, $modalInstance, informer, $rootScope, $timeout, uuid, $filter) {
            
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
                    // {id: uuid.generate(), name: 'Проверенные'},
                    // {id: uuid.generate(), name: 'Черный список'},
                    // {id: uuid.generate(), name: 'Магазины'},
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
                var contacts = $filter('filter')($rootScope.settings.contacts, group.id);
                $scope.group_to_delete.contacts = contacts;
            }

            $scope.realRemoveGroup = function(group){
                var gid = angular.copy(group.id);
                var contacts = angular.copy(group.contacts);
                angular.forEach($rootScope.settings.contact_groups, function(item, index){
                    // alert(JSON.stringify(item.id)+' ----->>>>> '+JSON.stringify(group.id));
                    if(item.id == group.id){
                        $rootScope.settings.contact_groups.splice(index,1);
                        angular.forEach(contacts,function(contact){
                            var gid_index = contact.group_ids.indexOf(group.id);
                            if(gid_index > -1){
                                contact.group_ids.splice(gid_index,1); // delete id of group from all contacts
                            }
                        });
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

            $scope.deleteContact = function(contact){
                angular.forEach($rootScope.settings.contacts, function(item, index){
                    if(item.id == contact.id){
                        $rootScope.settings.contacts.splice(index,1);
                    }
                });
            }
        }
    ]);

    module.controller('addContactCtrl',['backend','$rootScope','$scope','informer', '$location','uuid',
        function(backend, $rootScope, $scope, informer, $location, uuid){
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
                addresses: []
            };

            $scope.im = {
                name: '',
                nickname: ''
            };

            $scope.new_address = '';

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
                contact.id = uuid.generate();
                $rootScope.settings.contacts.push(contact);
                $location.path('/contacts');
            }

            $scope.addAddress = function(address){
                $rootScope.settings.contacts.push(address);

            }

            $scope.selectAlias = function(obj){
                var alias = obj.originalObject;
                // $scope.contact.addresses[0] = alias.address;
                $scope.contact.is_valid_address = true;
                $scope.new_address = alias.address;
            }

            $scope.addAddress = function(){
                if($scope.new_address.length && $scope.contact.is_valid_address && $scope.contact.addresses.indexOf($scope.address) == -1){
                    $scope.contact.addresses.push($scope.new_address);
                    $scope.$broadcast('angucomplete-alt:clearInput'); // clear autocomplete input
                    $scope.collapse.new_address = false;
                    $scope.contact.is_valid_address = false;
                }
            }

            $scope.inputChanged = function(str){
                // delete $scope.transaction.alias;
                if(str.indexOf('@') != 0){
                    if(backend.validateAddress(str)){
                        $scope.contact.is_valid_address = true;
                        $scope.new_address = str;
                    }else{
                        $scope.contact.is_valid_address = false;
                        $scope.new_address = '';
                    }
                }else{
                    $scope.contact.is_valid_address = false;
                    $scope.new_address = '';
                }
            }
        }
    ]);



}).call(this);