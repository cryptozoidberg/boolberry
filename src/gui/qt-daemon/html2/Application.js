// Main object
Application = function(router, backend) {

    this.router = router;
    this.backend = backend;
    this.alreadyInitializedScreens = [];
    this.screens = [];
    this.settings = {
        maxWidgets: 20
    };
    var $app = this;
    
    this.init = function() {
        // Screens
        this.setUpScreens();

        // Init routing
        this.router.registerAppCallback(this.onScreenInit);
        this.router.loadPages(function() {
            // Some interface triggers, like selectpicker bootstrap plugin
            $app.initInterface();
        });
        this.router.registerEvents();

        // Start streaming updates from backend
        this.backend.application = this;
        this.backend.emulator.application = this;
        this.subscribeToNonBackendEvents();
        this.backend.onAppInit();
    };

    this.updateBackendInfoWidget = function() {
        var state = this.backend.last_daemon_state;
        if (state != null) {
            $('.widget[data-widget=backendInfo]').each(function() {
                var tbody = $(this).find('.backendInfoTable').find('tbody');
                tbody.html('');

                for(var key in state) {
                    var value = state[key];

                    // Make nested tables if this is an array
                    if ($.isArray(value)) {
                        var newValue = "<table class='table table-condensed table-bordered'>";
                        for(var _key in value) {
                            var _value = value[_key];
                            newValue += "<tr><td>";

                            // Oh my! Three nested tables!! That's just super crazy.
                            newValue += "<table class='table table-condensed table-bordered'>";
                            for(var __key in _value) {
                                var __value = _value[__key];

                                if (__key == 'date') {
                                    __value = $app.getDateFromTimestamp(__value);
                                }

                                newValue += "<tr><td>"+__key+"</td><td>"+__value+"</td></tr>";
                            }
                            newValue += "</table>";

                            newValue += "</tr></td>";
                        }
                        newValue += "</table>";
                        value = newValue;
                    }

                    var row = "<tr>";
                    row += "<td>" + key + "</td>";
                    row += "<td>" + value + "</td>";
                    row += "</tr>";

                    tbody.append(row);
                }
            });
        } else {
            console.log('backendInfo widget: state is null');
        }
    };

    this.setUpWidgets = function() {
        // Cache widgets

        $app.widgetCache = [];

        $('.widget').each(function() {
            var name = $(this).data('widget');
            $app.widgetCache[name] = $(this)[0].outerHTML;
        });

        // Remove them from DOM
        $('.widget').remove();

        // Register events
        $(document).on('click', '.buttonAddWidget', function() {
            $app.addWidget(this);
        });
        $(document).on('click', 'a[href="#removeThisWidget"]', function() {
            $app.removeWidget(this); return false;
        });
        $(document).on('click', 'a[href="#changeThisWidget"]', function() {
            $app.changeWidget(this); return false;
        });
        $(document).on('click', '.widget-header', function() {
            return false; // make headers non-clickable
        });
    };

    // Widget management
    this.addWidget = function(button) {
        var widget_number = this.getRandomInt();
        var widgetHTML = this.widgetCache['emptyWidget'];
        widgetHTML = '<div style="display: none;" class="widget-wrapper widget-wrapper-'+widget_number+'">' + widgetHTML + '</div>';
        $(button).before(widgetHTML);
        $('.widget-wrapper-'+widget_number).slideDown();
        this.widgetCountSanityCheck();
    };
    this.removeWidget = function(element) {
        $(element).parents('.widget-wrapper').slideUp(function() {
            $(this).remove();
        });
        this.widgetCountSanityCheck();
    };
    this.widgetCountSanityCheck = function() {
        var count = $('.widget').length;
        var button = $('.buttonAddWidget');
        (count >= this.settings.maxWidgets) ? button.hide() : button.show();
    };
    this.changeWidget = function(element) {
        var widgetName = $(element).data('target-widget');
        var widgetHTML = this.widgetCache[widgetName];
        $(element).parents('.widget')
            .before(widgetHTML)  // adding that
            .remove();           // and removing this

        // Patches
        if (widgetName == 'activeMining') {
            doPlot("right"); // draw charts
        }
        if (widgetName == 'backendInfo') {
            this.updateBackendInfoWidget();
        }
    };

    // Mostly init events for different screens
    this.setUpScreens = function() {
        this.screens['index'] = {
            init: function() {
                $app.setUpWidgets(); // todo: redo when no emulator
            }
        };
        this.screens['deposits'] = {
            init: function() {
                initMiningDiagrams();
            }
        };
        this.screens['settings'] = {
            init: function() {
                // Make section headers unclickable
                $('a[data-toggle="collapse-off"]').attr('onclick', 'return false;');
            }
        };
        this.screens['buyG'] = {
            init: function() {

            }
        };
    };

    this.initInterface = function() {
        $('.popover-name').popover();
        $('.selectpicker').selectpicker({
            'selectedText': 'cat'
        });
        $(".bs-ui-slider").slider({});
        $('a[data-toggle="collapse"]').attr('onclick', 'return false;'); // Make section headers unclickable

        // Buttons & actions
        $(document).on("click", ".index_open-safe-button", function() {
            var fileName = $app.backend.showOpenFileDialog("Выберите файл сейфа", function(status, param) { // todo: translate
                if (status.error_code == "OK" && param.path && param.path != '') {
                    $app.showModal('#openSafe', 'open-safe', 'screen-1', true); // clears all inputs beforehand
                    $("input[name='open-safe_file-name-input']").val( param.path );
                } else {
                    console.log("OpenFileDialog API Error: "+status.error_code);
                }
            });
        });
        $(document).on("click", ".open-safe_step-2-button", function() {
            $app.showModalScreen('open-safe', 'screen-2');
            return false;
        });
        $(document).on("click", ".open-safe_step-1-button", function() {
            $app.showModalScreen('open-safe', 'screen-1');
            return false;
        });
        $(document).on("click", ".open-safe_open-safe-button", function() {
            var file = $('input[name=open-safe_file-name-input]').val();
            var name = $('input[name=open-safe_safe-name-input]').val();
            var pass = $('input[name=open-safe_safe-password-input]').val();

            loadingAnimationOn();
            $app.backend.openWallet(file, name, pass, function(status, data) {
                if (status.error_code != 'OK') {
                    alert('Backend Error: '+status.error_code); // todo: replace alert

                    loadingAnimationOff();
                    $app.hideModal('#openSafe');

                } else {
                    // OK, load the wallet
                    $app.backend.loadWalletToClientSide(data.wallet_id, function(status, param) {
                        if (status.error_code != 'OK') {
                            alert('Backend Error: '+status.error_code); // todo: replace alert
                        } else {
                            // OK! Backend will fire an event to show the safe
                        }

                        loadingAnimationOff();
                        $app.hideModal('#openSafe');
                    });
                }
            });

            return false;
        });
        $(document).on("click", ".index_close-safe-button", function() {
            loadingAnimationOn();
            var safe_id = $(this).parents('.safebox').data('safe-id');
            $app.backend.closeWallet(safe_id, function(status, data) {
                if (status.error_code != "OK") {
                    alert("Backend Error: "+status.error_code); // todo: replace alert
                }

                loadingAnimationOff();
            });
            $('.safebox').click();
            return false;
        });

        // Premium/standard placement radiobox
        $(document).on("click", "[id$=_premiumPlacement_checkbox]", function() {
            var opposite_id = $(this).attr('id').replace('premium', 'standard');
            $('#'+opposite_id).trigger('click');
        });
        $(document).on("click", "[id$=_standardPlacement_checkbox]", function() {
            var opposite_id = $(this).attr('id').replace('standard', 'premium');
            $('#'+opposite_id).trigger('click');
        });

        // Remove update notification after some time
        setInterval(function() {
            $('.alert-warning').slideUp();
        }, 3000);

    };

    this.showOnlineState = function(state) {
        $('.ifOnlineText').hide();
        $('.ifOnlineText.'+ state.toLowerCase()).show();
    };

    // Renders the safe plate on the carousel on index screen
    this.reRenderSafe = function(safe_id) {
        var safe = $app.backend.safes[safe_id];

        var safe_html = $('.safebox_template').html();
        safe_html = safe_html.replace('{{ address }}', safe.address);
        safe_html = safe_html.replace('{{ address }}', safe.address); // not a mistake
        safe_html = safe_html.replace('{{ balance }}', safe.balance);
        safe_html = safe_html.replace('{{ label }}',   safe.wallet_id);
        safe_html = safe_html.replace('{{ safe_id }}', safe.wallet_id);

        $('.safebox[data-safe-id="'+safe_id+'"]').html(safe_html);
    };

    this.subscribeToNonBackendEvents = function() {
        this.backend.subscribe("update_safe_count", function() {
            $app.updateSafeCarousel();
            $app.updateSafeCounters();
        });
        this.backend.subscribe("update_balance", function() {
            $app.updateBalanceCounters();
        });
    };

    // Updates counters on index screen
    this.updateSafeCounters = function() {
        // Calculate total number of safe (counter on index screen)
        $('.index_safeCount').text(Object.size(this.backend.safes));

        // And overall balance is likely to change too
        this.updateBalanceCounters();
    };
    
    // Overall balance
    this.updateBalanceCounters = function() {
        // Calculate total balance (label on index screen)
        var total_balance = 0;
        for (var j in this.backend.safes) {
            total_balance += this.backend.safes[j].balance;
        }
        $('.index_totalBalance').text(total_balance);
    };

    // Rebuilds the carousel on index screen entirely
    this.updateSafeCarousel = function() {
        // Destroy if first
        owlSafes.trigger('destroy.owl.carousel');
        owlSafes.html('');

        // Now add the items
        for (var i in this.backend.safes) {
            var safe = this.backend.safes[i];
            var safe_html = $('.safebox_empty_template').html();
            safe_html = safe_html.replace('{{ safe_id }}', safe.wallet_id);
            owlSafes.append(safe_html);

            // Insert actual elements
            this.reRenderSafe(safe.wallet_id);
        }

        // And... rebuild the carousel
        owlSafes.owlCarousel({
            items: 2,
            navText: '',
            margin: 10
        });
    };

    this.onScreenInit = function(screen) {
        if ($app.alreadyInitializedScreens.indexOf(screen) < 0) {
            $app.alreadyInitializedScreens.push(screen);
            if ($app.screens[screen]) {
                $app.screens[screen].init();
            }
        }

        $app.backend.updateScreen(screen);
    };

    this.showModal = function(selector, name, screen, clearInputs) {
        $(selector).modal('show');
        this.showModalScreen(name, screen);

        if (clearInputs) {
            $(selector).find('input[type=text],[type=password],[type=hidden]').val("");
        }
    };
    this.hideModal = function(selector) {
        $(selector).modal('hide');
    };
    this.showModalScreen = function(name, screen){
        $('.'+name+'_screen').hide();
        $('.'+name+'_'+screen).show()
    };

    this.getRandomInt = function(min, max) {
        return Math.floor(Math.random() * (max - min + 1) + min);
    };
    this.getRandomObjectProperty = function(obj) {
        var keys = Object.keys(obj)
        return obj[keys[ keys.length * Math.random() << 0]];
    };

    // Unix timestamp to formatted date/time string
    // - stolen from: http://stackoverflow.com/questions/847185/convert-a-unix-timestamp-to-time-in-javascript
    //   & http://stackoverflow.com/questions/3552461/how-to-format-javascript-date
    this.getDateFromTimestamp = function(unix_timestamp) {
        // create a new javascript Date object based on the timestamp
        // multiplied by 1000 so that the argument is in milliseconds, not seconds
        var date = new Date(unix_timestamp*1000);

        var m_names = new Array("January", "February", "March",
            "April", "May", "June", "July", "August", "September",
            "October", "November", "December");

        var d = new Date();
        var curr_date = d.getDate();
        var curr_month = d.getMonth();
        var curr_year = d.getFullYear();

        // hours part from the timestamp
        var hours = "0" + date.getHours();
        // minutes part from the timestamp
        var minutes = "0" + date.getMinutes();
        // seconds part from the timestamp
        var seconds = "0" + date.getSeconds();

        // will display time in 10:30:23 format
        var formattedTime = curr_date + "-" + m_names[curr_month] + "-" + curr_year + " " + hours.substr(hours.length-2) + ':' + minutes.substr(minutes.length-2) + ':' + seconds.substr(seconds.length-2);

        return formattedTime;
    };

    //////////////////////////////////////////////////////////////

    this.init();

}; // -- end of Application definition

