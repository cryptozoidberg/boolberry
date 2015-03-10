// Backend emulation
Emulator = function() {

    this.backend = null; // set in Backend
    this.settings = {
        asyncEmulationTimeout: 500
    };
    var lastRequestId = 0;
    var $emulator = this;

    // Should return a function to be called with the arguments later
    this.backendRequestCall = function(command) {
        if (command in this.functions) {
            return function(/*arguments*/) {
                // Make up request ID
                var requestId = ++lastRequestId;

                // Emulate async call using timeout
                setTimeout(function(arg)
                    {
                        // Actually calling the command with arguments of parent function
                        var returnObject = $emulator.functions[arg.command].apply(this, arg.arguments);

                        // Passing the emulated result to our UI callback
                        var status = {
                            error_code: returnObject.error_code,
                            request_id: arg.requestId
                        };
                        $emulator.sendAnswer(status, returnObject.data);
                    },
                    $emulator.settings.asyncEmulationTimeout,
                    {command: command, requestId: requestId, arguments: arguments}
                );

                // Successful API call
                return JSON.stringify({
                    error_code: "OK",
                    request_id: requestId
                });
            }
        } else {
            // There's no such command
            return function(/*arguments*/) {
                return JSON.stringify({
                    error_code: "WRONG_API_COMMAND"
                });
            };
        }
    };

    this.sendAnswer = function(status, data) {
        dispatch(JSON.stringify(status), JSON.stringify(data));
    };

    this.functions = {
        'onlineStatus': function() {
            var status = (Math.random() >= 0.7) ? 'online' : 'offline'; // and 'loading'
            return {error_code: "OK", data: {status: status}};
        },
        'safeCount': function() {
            var count = (Math.random() >= 0.7) ? '5' : '6';
            return {error_code: "OK", data: {count: count}};
        },
        'paymentCount': function() {
            var count = (Math.random() >= 0.7) ? '35' : '40';
            return {error_code: "OK", data: {count: count}};
        },
        'orderCount': function() {
            var count = (Math.random() >= 0.7) ? '4' : '3';
            return {error_code: "OK", data: {count: count}};
        },
        'totalBalance': function() {
            var balance = (Math.random() >= 0.7) ? '1240,16' : '1264,00';
            return {error_code: "OK", data: {balance: balance}};
        }
    };

}; // -- end of Emulator