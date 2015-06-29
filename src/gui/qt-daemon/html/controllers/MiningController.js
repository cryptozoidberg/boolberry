(function() {
    'use strict';
    var module = angular.module('app.mining',[]);

    module.controller('minigHistoryCtrl',['backend','$rootScope','$scope','informer','$routeParams','$filter','$location','emulator',
        function(backend,$rootScope,$scope,informer,$routeParams,$filter,$location,emulator){

            //informer.info('mining');
            $scope.mining_history = [];

            $scope.safes_diagramm = {};

            

            $scope.calc = {
                days: 0,
                amount: 0
            };

            var getPoints = function () {
                var nextDate = new Date().getTime();

                var points = [];

                var total = 0;

                for(var i=1; i!=20; i++){
                    var step = Math.random()*10;
                    total += step;
                    points.push([new Date(nextDate).getTime(), total]);
                    nextDate += 1000*60*60*24;
                }

                return points;
            };
            

            $scope.calculate = function(){
                
                var result = backend.getMiningEstimate($scope.calc.amount, parseInt($scope.calc.days)*60*60*24);

                var points = [];

                var days = result.days_estimate;
                days = [123200000000, 143200000000, 163200000000, 193200000000, 223200000000];

                var nextDate = new Date().getTime();

                angular.forEach(days,function(item){
                    points.push([nextDate, item]);
                    nextDate += 1000*60*60*24;
                });

                $scope.calc_diagramm = [{
                    data: points,
                    label: 'Прогнозируемый доход'
                }];

            }

            //$scope.calculate();

            angular.forEach($rootScope.safes,function(safe){
                
                var diagramm = {
                    label: safe.name,
                    data: []
                };

                backend.getMiningHistory(safe.wallet_id, function(data){
                    // informer.info(JSON.stringify(data));
                    var total = 0;
                    angular.forEach(data.mined_entries,function(item){
                        total += item.a;
                        diagramm.data.push([item.t * 1000, $filter('gulden')(total, false)]);
                    });
                    $scope.safes_diagramm[safe.address] = [diagramm];
                });

                
            }); 

            var position = "right";

            var euroFormatter = function (v, axis) {
                return v.toFixed(axis.tickDecimals) + "G";
            }

            $scope.am = {};

            $scope.diagramm_options = {
                xaxes: [{
                    mode: 'time'
                }],
                yaxes: [{
                    min: 0
                }, {
                    // align if we are to the right
                    alignTicksWithAxis: position == "right" ? 1 : null,
                    position: position,
                    tickFormatter: euroFormatter
                }],
                legend: {
                    position: 'sw'
                },
                colors: ["#1ab394"],
                grid: {
                    color: "#999999",
                    clickable: true,
                    tickColor: "#D4D4D4",
                    borderWidth:0,
                    hoverable: true //IMPORTANT! this is needed for tooltip to work,

                },
                tooltip: true,
                tooltipOpts: {
                    content: "%s for %x was %y",
                    xDateFormat: "%y-%m-%d",

                    onHover: function(flotItem, $tooltipEl) {
                        // console.log(flotItem, $tooltipEl);
                    }
                }
            };


            var euroFormatter = function (v, axis) {
                return v.toFixed(axis.tickDecimals) + "G";
            }

            $scope.am.data = [{
                data: emulator.getAMData(),
                label: "Safelabel1"
            }, {
                data: emulator.getAMData(),
                label: "Safelabel2",
                // yaxis: 2
            }];
            var position = "right";
            $scope.am.options = {
                xaxes: [{
                    mode: 'time'
                }],
                yaxes: [{
                    min: 0
                }, {
                    // align if we are to the right
                    alignTicksWithAxis: position == "right" ? 1 : null,
                    position: position,
                    tickFormatter: euroFormatter
                }],
                legend: {
                    position: 'sw'
                },
                colors: ["#1ab394"],
                grid: {
                    color: "#999999",
                    clickable: true,
                    tickColor: "#D4D4D4",
                    borderWidth:0,
                    hoverable: true //IMPORTANT! this is needed for tooltip to work,

                },
                tooltip: true,
                tooltipOpts: {
                    content: "%s for %x was %y",
                    xDateFormat: "%y-%0m-%0d",

                    onHover: function(flotItem, $tooltipEl) {
                        // console.log(flotItem, $tooltipEl);
                    }
                }
            };

        }
    ]);

}).call(this);