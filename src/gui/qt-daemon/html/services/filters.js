(function() {
    'use strict';

    var module = angular.module('app.filters', []);

	module.filter('gulden',[function(){
		return function(input, need_g){
			if(angular.isUndefined(need_g)){
				need_g = true;
			}

			if(angular.isDefined(input)){
				input = input.toString();

				var CDDP = 6; //CURRENCY_DISPLAY_DECIMAL_POINT
				var result = '';
				if(input.length ){
					result = angular.copy(input);
					while (result.length < (CDDP+1)){
						result = '0'+result;
					}

					result = result.substr(0, result.length - CDDP) + "." + result.substr(result.length - CDDP);
					var pos = 0;
					for (var i = (result.length-1); i > 0; i--) {
						if (result[i] == '0' && result[i-2] != '.'){
							pos = i;
						}else{
							break;
						}
					}
					if(pos){
						result = result.substr(0,pos);	
					}
					if(need_g){
						result += ' G';	
					}
					

				}
				return result;	
			}
			
		}
	}]);

	module.filter('intToDate',[function(){
		return function(input){
			if(angular.isDefined(input)){
				input = parseInt(input);
				var result = '';
				if(!isNaN(input)){
					var date = new Date(input*1000);
					result = date;
				}
				return result;
			}
			
		}
	}]);

}).call(this);