// Talks to C++ backend
Backend = function(emulator) {

    this.settings = {
        useEmulator: true,
        intervalCheckIfOnline: 5000,
        intervalUpdateCurrentScreen: 5000
    };
    this.emulator = emulator;
    this.emulator.backend = this;
    var callbacks = [];
    var currentScreen = null;
    var $backend = this;

    /******************** 'Talking to backend' stuff **********************/

    // Requests a function
    this.backendRequest = function(command, parameters, callback) {
        // Emulated call versus real one through the magic Qt object
        var commandFunction = (this.settings.useEmulator) ? this.emulator.backendRequestCall(command) : Qt[command];

        // Now call it
        var returnValue = commandFunction(parameters);
        var returnObject = JSON.parse(returnValue);

        if (returnObject.error_code != "OK") {
            // TODO: get rid of native alert window
            // TODO: do we really need any kind of alert here? console.log maybe?
            alert("API Error: " + returnObject.error_code);
        } else {
            // Everything is OK
            callbacks[returnObject.request_id] = callback;
        }
    };

    // Callback from backend
    this.backendCallback = function(status, param) {
        // Everything is OK

        // Do we have a callback for this?
        var requestId = status.request_id;
        if (callbacks[requestId]) {
            var callback = callbacks[requestId];
            callback(status, param);
        } else {
            // TODO: backend taking the initiative of talking to interface, like on_update_wallet_info
            // TODO: if not the case, log an error? that's what we do, for now, we'll dump the error into the console
            console.log("API Error: no such request_id: ["+requestId+"]");
        }
    };

    // Set global function shortcut for backend
    dispatch = function(status, param) {
        status = JSON.parse(status);
        param = JSON.parse(param);
        return $backend.backendCallback(status, param);
    };

    /******************** Specific backend API calls being reflected in UI *********************/

    // Register callbacks for automatic events from backend side
    this.registerEventCallbacks = function() {
        // TODO
    };

    // UI initialization
    this.onAppInit = function() {
        this.registerEventCallbacks();

        // -- // -- // -- // -- //

        setInterval(function() {
            $backend.checkIfOnline();
        }, this.settings.intervalCheckIfOnline);

        $backend.checkIfOnline();

        setInterval(this.updateCurrentScreen, this.settings.intervalUpdateCurrentScreen);
    };

    this.updateCurrentScreen = function() {
        if (currentScreen != null) $backend.updateScreen(currentScreen);
    };

    this.updateScreen = function(screen) {
        currentScreen = screen;
        switch(screen) {
            case 'index':
                this.backendRequest('safeCount', null, function(status, data) {
                    var count = (status.error_code == "OK") ? data.count : 'Error'; // todo: translate
                    $('.index_safeCount').text(count);
                });
                this.backendRequest('paymentCount', null, function(status, data) {
                    var count = (status.error_code == "OK") ? data.count : 'Error'; // todo: translate
                    $('.index_paymentCount').text(count);
                });
                this.backendRequest('totalBalance', null, function(status, data) {
                    var balance = (status.error_code == "OK") ? data.balance : 'Error'; // todo: translate
                    $('.index_totalBalance').text(balance);
                });
                this.backendRequest('orderCount', null, function(status, data) {
                    var count = (status.error_code == "OK") ? data.count : 'Error'; // todo: translate
                    $('.index_orderCount').text(count);
                });
                break;
        }
    };

    this.checkIfOnline = function() {
        this.backendRequest('onlineStatus', null, function(status, data) {
            var status = (status.error_code == "OK") ? data.status : 'Error'; // todo: translate

            $('.ifOnlineText').hide();
            $('.ifOnlineText.'+status).show();
        });
    };

}; // -- end of Backend definition