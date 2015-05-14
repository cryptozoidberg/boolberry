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

    module.filter('gulden_to_int',[function(){
        return function(input){

            var CURRENCY_DISPLAY_DECIMAL_POINT = 6;

            var append_string_with_zeros = function (result, len){

                 for(var i = 0; i != len; i++)
                 {
                     result = result+'0';
                 }
                return result;
            }

            var result = false;

            if(angular.isDefined(input)){
                input = input.toString();
                
                console.log("Parsing: " + input);
                var am_str = input.trim();

                var point_index = am_str.indexOf('.');
                var fraction_size = 0;
                var result = 0;
                console.log("point_index = " + point_index);

                if (-1 != point_index){
                    fraction_size = am_str.length - point_index - 1;
                    while (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size && '0' == am_str[am_str.length-1]){
                        am_str = am_str.slice(0, str_amount.size() - 1);//erase(str_amount.size() - 1, 1);
                        --fraction_size;
                    }
                    
                    console.log("fraction_size = " + fraction_size);

                    if (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size)
                    return undefined;

                    console.log("am_str before point remove = " + am_str);        
                    am_str = am_str.slice(0, point_index) + am_str.slice(point_index + 1, am_str.length);
                    console.log("am_str after point remove = " + am_str);        
                }
                else{
                  fraction_size = 0;
                }

                if (!am_str.length)
                    return undefined;

                if (fraction_size < CURRENCY_DISPLAY_DECIMAL_POINT){
                    am_str = append_string_with_zeros(am_str, CURRENCY_DISPLAY_DECIMAL_POINT - fraction_size);
                }
                console.log("Parsing final string " + am_str);
                result = parseInt(am_str); 
                console.log("string " + am_str + " to " + result);
            }

            return result;
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