// Main object
Application = function(router, backend) {

    this.router = router;
    this.backend = backend;
    this.alreadyInitializedScreens = [];
    this.screens = [];
    this.settings = {
        maxWidgets: 12,
        userSettings: {
            'sendG_showPassword': true
        }
    };
    var $app = this;
    
    this.init = function() {
        // Screens
        this.setUpScreens();

        // Init routing
        this.router.application = this;
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
                            // Well, it's only for testing purposes, so...
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

        // Show backend info widget
        $('.buttonAddWidget:first').click();
        $('.widget:first a[data-target-widget=backendInfo]').click();
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
        this.screens['market'] = {
            init: function() {
                $('.market_export-my-orders').hide();
                $('.market_tabs').click(function() {
                    if ($(this).attr('href')=='#myOrders') {
                        $('.market_export-my-orders').show();
                    } else {
                        $('.market_export-my-orders').hide();
                    }
                });
            }
        };
        this.screens['contacts'] = {
            init: function() {
                $app.updateContactBook();
            }
        };
        this.screens['contact'] = {
            init: function() {
                $('.contact_add-messenger-button').click(function() {
                    if ($('.contact_ims .im').length < 10) {
                        $('.contact_ims').append($('.appScreen_contact .im')[0].outerHTML);
                    }
                });
                $(document).on('click', '.contact_im-delete-button', function() {
                    $(this).parents('.im').slideUp(function() {
                        $(this).remove();
                    });
                });
            }
        };
        this.screens['buyG'] = {
            init: function() {
                $('.buyG_amountG, .buyG_amountFiat, .buyG_rate').numeric();
                $('.buyG_amountG, .buyG_amountFiat, .buyG_rate').keyup(function() {
                    if ($(this).val().indexOf(',') >= 0) {
                        $(this).val($(this).val().replace(/,/g, '.'));
                    }
                });
                $('.buyG_amountG').keyup(function() {
                    var G = parseFloat($('.buyG_amountG').val());
                    var fiat = parseFloat($('.buyG_amountFiat').val());
                    var rate = parseFloat($('.buyG_rate').val());
                    if (fiat != 0 && !isNaN(fiat)) {
                        rate = fiat / G;
                    } else if (rate != 0 && !isNaN(rate)) {
                        fiat = G * rate;
                    } else {
                        fiat = 0;
                        rate = 0;
                    }
                    $('.buyG_amountFiat').val(isNaN(fiat) ? 0 : fiat);
                    $('.buyG_rate').val(isNaN(rate) ? 0 : rate);
                });
                $('.buyG_amountFiat').keyup(function() {
                    var G = parseFloat($('.buyG_amountG').val());
                    var fiat = parseFloat($('.buyG_amountFiat').val());
                    var rate = parseFloat($('.buyG_rate').val());
                    if (G != 0 && !isNaN(G)) {
                        rate = fiat / G;
                    } else if (rate != 0 && !isNaN(rate)) {
                        G = fiat / rate;
                    } else {
                        G = 0;
                        rate = 0;
                    }
                    $('.buyG_amountG').val(isNaN(G) ? 0 : G);
                    $('.buyG_rate').val(isNaN(rate) ? 0 : rate);
                });
                $('.buyG_rate').keyup(function() {
                    var G = parseFloat($('.buyG_amountG').val());
                    var fiat = parseFloat($('.buyG_amountFiat').val());
                    var rate = parseFloat($('.buyG_rate').val());
                    if (G != 0 && !isNaN(G)) {
                        fiat = G * rate;
                    } else if (fiat != 0 && !isNaN(rate)) {
                        G = fiat / rate;
                    } else {
                        fiat = 0;
                        rate = 0;
                    }
                    $('.buyG_amountG').val(isNaN(G) ? 0 : G);
                    $('.buyG_amountFiat').val(isNaN(fiat) ? 0 : fiat);
                });
            },
            show: function() {
                $('.buyG_sticker').sticky({topSpacing: 80});
            },
            hide: function() {
                $('.buyG_sticker').unstick();
            }
        };
        this.screens['sellG'] = {
            init: function() {
                $('.sellG_amountG, .sellG_amountFiat, .sellG_rate').numeric();
                $('.sellG_amountG').keyup(function() {
                    var G = parseFloat($('.sellG_amountG').val());
                    var fiat = parseFloat($('.sellG_amountFiat').val());
                    var rate = parseFloat($('.sellG_rate').val());
                    if (fiat != 0 && !isNaN(fiat)) {
                        rate = fiat / G;
                    } else if (rate != 0 && !isNaN(rate)) {
                        fiat = G * rate;
                    } else {
                        fiat = 0;
                        rate = 0;
                    }
                    $('.sellG_amountFiat').val(isNaN(fiat) ? 0 : fiat);
                    $('.sellG_rate').val(isNaN(rate) ? 0 : rate);
                });
                $('.sellG_amountFiat').keyup(function() {
                    var G = parseFloat($('.sellG_amountG').val());
                    var fiat = parseFloat($('.sellG_amountFiat').val());
                    var rate = parseFloat($('.sellG_rate').val());
                    if (G != 0 && !isNaN(G)) {
                        rate = fiat / G;
                    } else if (rate != 0 && !isNaN(rate)) {
                        G = fiat / rate;
                    } else {
                        G = 0;
                        rate = 0;
                    }
                    $('.sellG_amountG').val(isNaN(G) ? 0 : G);
                    $('.sellG_rate').val(isNaN(rate) ? 0 : rate);
                });
                $('.sellG_rate').keyup(function() {
                    var G = parseFloat($('.sellG_amountG').val());
                    var fiat = parseFloat($('.sellG_amountFiat').val());
                    var rate = parseFloat($('.sellG_rate').val());
                    if (G != 0 && !isNaN(G)) {
                        fiat = G * rate;
                    } else if (fiat != 0 && !isNaN(rate)) {
                        G = fiat / rate;
                    } else {
                        fiat = 0;
                        rate = 0;
                    }
                    $('.sellG_amountG').val(isNaN(G) ? 0 : G);
                    $('.sellG_amountFiat').val(isNaN(fiat) ? 0 : fiat);
                });
            },
            show: function() {
                $('.sellG_sticker').sticky({topSpacing: 80});
            },
            hide: function() {
                $('.sellG_sticker').unstick();
            }
        };

    };

    // Show screen option (triggered by path in hash, like #screenName/option)
    this.showScreenOption = function(screen, option, router) {
        switch(screen) {
            case 'history':
                switch(option) {
                    case 'ingoing':
                        $('.history_filterIngoingButton').click(); break;
                    case 'outgoing':
                        $('.history_filterOutgoingButton').click(); break;
                    case 'all':
                        $('.history_filterIngoingOutgoingButton').click(); break;
                }
                break;
            case 'market':
                switch(option) {
                    case 'currency/bid':
                        $('.market_filterCurrencyButton').click();
                        $('.market_filterCurrencyBidButton').click();
                        break;
                    case 'currency/ask':
                        $('.market_filterCurrencyButton').click();
                        $('.market_filterCurrencyAskButton').click();
                        break;
                    case 'barter/bid':
                        $('.market_filterBarterButton').click();
                        $('.market_filterBarterBidButton').click();
                        break;
                    case 'barter/ask':
                        $('.market_filterBarterButton').click();
                        $('.market_filterBarterAskButton').click();
                        break;
                    case 'favorites/currency':
                        $('.market_filterMyFavoritesButton').click();
                        $('.market_filterMyFavoritesCurrencyButton').click();
                        $('.market_filterMyFavoritesAllButton').click();
                        break;
                    case 'my/currency':
                        $('.market_filterMyOrdersButton').click();
                        $('.market_filterMyOrdersCurrencyButton').click();
                        $('.market_filterMyOrdersAllButton').click();
                        break;
                    case 'favorites/barter':
                        $('.market_filterMyFavoritesButton').click();
                        $('.market_filterMyFavoritesBarterButton').click();
                        $('.market_filterMyFavoritesAllButton').click();
                        break;
                }
                break;
            case 'contact':
                var contact_id = option;
                // todo: what if contacts didn't yet load?
                if ($app.backend.contacts && $app.backend.contacts[contact_id]) {
                    var c = $app.backend.contacts[contact_id];
                    $('.contact_mainArea').show();
                    $('.contact_errorArea').hide();
                } else {
                    $('.contact_mainArea').hide();
                    $('.contact_errorArea').show();
                    return;
                }

                // Prepare to load
                var group_options = '';
                for(var g in $app.backend.contactGroups) {
                    var group = $app.backend.contactGroups[g];
                    var selected = (c.groups.indexOf(group) >= 0) ? 'selected' : '';
                    group_options += '<option '+selected+'>' + $app.backend.contactGroups[g] + '</option>';
                }

                // Load contact
                var html = $('.contact_template').html();
                html = html.replace(/{{ contact.id }}/g, contact_id);
                html = html.replace(/{{ contact.name }}/g, c.name);
                html = html.replace(/{{ contact.contactFields.Email }}/g, c.contactFields.Email);
                html = html.replace(/{{ contact.contactFields.Skype }}/g, c.contactFields.Skype);
                html = html.replace(/{{ contact.contactFields.Facebook }}/g, c.contactFields.Facebook);
                html = html.replace(/{{ contact.location.country }}/g, c.location.country);
                html = html.replace(/{{ contact.location.city }}/g, c.location.city);
                html = html.replace(/{{ contact.note }}/g, c.note);
                html = html.replace(/{{ contact.account }}/g, c.account);
                html = html.replace(/{{ contact.alias1 }}/g, c.aliases[0]);
                html = html.replace(/{{ contact.alias2 }}/g, c.aliases[1]);
                html = html.replace(/{{ group_options }}/g, group_options);
                html = html.replace(/{{ selectpicker_class }}/g, 'selectpicker');
                $('.contact_insertion').html(html);

                $('.appScreen_contact .selectpicker').selectpicker();

                break;
        }
    };

    this.initInterface = function() {
        $('.popover-name').popover();
        $('.selectpicker').selectpicker({
            'selectedText': 'cat'
        });
        $(".bs-ui-slider").slider({});
        $('a[data-toggle="collapse"]').attr('onclick', 'return false;'); // Make section headers unclickable

        // Buttons & actions
        $(document).on("click", ".safes_open-safe-button, .index_open-safe-button", function() {
            var param = $app.backend.showOpenFileDialog("Выберите файл сейфа"); // todo: translate
            if (param.error_code == "OK" && param.path && param.path != '') {
                $app.showModal('#openSafe', 'open-safe', 'screen-1', true); // clears all inputs beforehand
                $("input[name='open-safe_file-name-input']").val( param.path );
            } else {
                console.log("OpenFileDialog API Error: "+param.error_code);
            }
        });
        $(document).on("click", ".buyG_submitButton", function() {
            $app.showModal('#confirm_operation_modal', 'confirm-modal', 'buyG');
            return false;
        });
        $(document).on("click", ".sellG_submitButton", function() {
            $app.showModal('#confirm_operation_modal', 'confirm-modal', 'sellG');
            return false;
        });
        $(document).on("click", ".buyProduct_submitButton", function() {
            $app.showModal('#confirm_operation_modal', 'confirm-modal', 'buyProduct');
            return false;
        });
        $(document).on("click", ".sellProduct_submitButton", function() {
            $app.showModal('#confirm_operation_modal', 'confirm-modal', 'sellProduct');
            return false;
        });
        $(document).on("click", ".sendG_submitButton", function() {
            $app.showModal('#confirm_operation_modal', 'confirm-modal', 'sendG');
            if ($app.settings.userSettings.sendG_showPassword) {
                $('.confirm-modal_sendG-password-field').show();
            } else {
                $('.confirm-modal_sendG-password-field').hide();
            }
            return false;
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
        $(document).on("click", ".index_create-safe-button, .safes_create-safe-button", function() {
            $app.showModal('#addSafe', 'create-safe', 'screen-1', true); // clears all inputs beforehand
        });
        $(document).on("click", ".create-safe_step-1-button", function() {
            $app.showModalScreen('create-safe', 'screen-1');
            return false;
        });
        $(document).on("click", ".create-safe_step-2-button", function() {
            $app.showModalScreen('create-safe', 'screen-2');
            return false;
        });
        $(document).on("click", ".create-safe_step-3-button", function() {
            $app.showModalScreen('create-safe', 'screen-3');
            return false;
        });
        $(document).on("click", ".create-safe_step-4-button", function() {
            $app.showModalScreen('create-safe', 'screen-4');
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
            var checked = $(this).is(':checked');
            var opposite_id = $(this).attr('id').replace('premium', 'standard');
            var opposite_checked = $('#'+opposite_id).is(':checked');
            if (checked && opposite_checked) {
                $('#'+opposite_id).click();
            } else if (!checked && !opposite_checked) {
                $('#'+opposite_id).click();
            }
        });
        $(document).on("click", "[id$=_standardPlacement_checkbox]", function() {
            var checked = $(this).is(':checked');
            var opposite_id = $(this).attr('id').replace('standard', 'premium');
            var opposite_checked = $('#'+opposite_id).is(':checked');
            if (checked && opposite_checked) {
                $('#'+opposite_id).click();
            } else if (!checked && !opposite_checked) {
                $('#'+opposite_id).click();
            }
        });

        // Radiobuttons
        var selector = 'input[type=radio][name=marketMyOrdersRadio]';
        $(document).on("change", selector, function() {
            var val = $(selector + ':checked').val();
            if (val == 'currency') {
                $('#myCurrency').show();
                $('#myBarter').hide();
            } else {
                $('#myBarter').show();
                $('#myCurrency').hide();
            }
        });
        $(function() { $(selector).change(); });
        // Radiobuttons
        var selector2 = 'input[type=radio][name=marketMyFavoritesRadio]';
        $(document).on("change", selector2, function() {
            var val = $(selector2 + ':checked').val();
            if (val == 'currency') {
                $('#favCurrency').show();
                $('#favBarter').hide();
            } else {
                $('#favBarter').show();
                $('#favCurrency').hide();
            }
        });
        $(function() { $(selector2).change(); });

        // Settings
        $(document).on('change', '[name=options_sendG_showPassword]', function() {
            var value = $('[name=options_sendG_showPassword]:checked').val();
            $app.settings.userSettings.sendG_showPassword = (value === 'true');
        });
        $(function() {
            $('[name=options_sendG_showPassword]').trigger('change');
        });

        // Tooltips for help
        $('input[name=options2_1]').change(function() {
            if ($('input[name=options2_1]:checked').attr('id')=='option1') {
                $('button.popover-name').css({'visibility': 'visible'});
            } else {
                $('button.popover-name').css({'visibility': 'hidden'});
            }
        });

        // Remove update notification after some time
        setInterval(function() {
            $('.alert-warning').slideUp();
        }, 3000);

        // History and Market screen - collapse filters
        setTimeout(function() {
            //if ($('.appScreen_history .close-filter.filter1').attr('aria-expanded')==='true') {
            //    $('.appScreen_history .close-filter.filter1 .btn[data-toggle=collapse]').click();
            //}
            //if ($('.appScreen_market .close-filter.filter11').attr('aria-expanded')==='true') {
            //    $('.appScreen_market .close-filter.filter11 .btn[data-toggle=collapse]').click();
            //}
        }, 500);

        // Datepickers
        $('.appScreen_history .input-daterange').datepicker({
            autoclose: true,
            todayHighlight: true,
            language: "ru"
        });
        $('.appScreen_market .input-date').datepicker({
            autoclose: true,
            todayHighlight: true,
            language: "ru"
        });
        $('.market_currency_order-age-selector').change(function() {
            if ($(this).val() == 'другой период') {
                $('.market_currency_order-age-datapicker-row').show();
            } else {
                $('.market_currency_order-age-datapicker-row').hide();
            }
        });
        $('.market_currency_order-age-selector').change();
        $('.market_barter_order-age-selector').change(function() {
            if ($(this).val() == 'другой период') {
                $('.market_barter_order-age-datapicker-row').show();
            } else {
                $('.market_barter_order-age-datapicker-row').hide();
            }
        });
        $('.market_barter_order-age-selector').change();
        $('.history_period-selector').change(function() {
            if ($(this).val() == 'другой период') {
                $('.history_period-datapicker-row').show();
            } else {
                $('.history_period-datapicker-row').hide();
            }
        });
        $('.history_period-selector').change();

        // Password strength visualizer
        $('#create-safe_safe-password-input-1').strengthMeter('progressBar', {
            container: $('.create-safe_password-strength-bar')
        });

        //// Autocomplete
        //$('.autocomplete-countries').autocomplete({
        //    minLength: 0,
        //    source: $app.countries,
        //    focus: function( event, ui ) {
        //        $(this).val( ui.item.name );
        //        return false;
        //    },
        //    select: function( event, ui ) {
        //        $(this).val( ui.item.name );
        //        $(this).data( 'selected-country-code', ui.item );
        //        return false;
        //    }
        //})
        //.autocomplete( "instance" )._renderItem = function( ul, item ) {
        //    return $( "<li>" )
        //        .append( "<a>" + item.name + "</a>" )
        //        .appendTo( ul );
        //};

    }; // end of initInterface function

    this.showOnlineState = function(state) {
        $('.ifOnlineText').hide().removeClass('hide');
        $('.ifOnlineText.'+ state.toLowerCase()).show();
    };

    // Renders safes plate on the carousel on index screen
    this.reRenderSafe = function(safe_id) {
        var safe = $app.backend.safes[safe_id];

        var safe_html = $('.safebox_template').html();
        safe_html = safe_html.replace('{{ address }}', safe.address);
        safe_html = safe_html.replace('{{ address }}', safe.address); // not a mistake
        safe_html = safe_html.replace('{{ balance }}', safe.balance);
        safe_html = safe_html.replace('{{ label }}',   safe.wallet_id);
        safe_html = safe_html.replace('{{ safe_id }}', safe.wallet_id);

        $('.safebox[data-safe-id="'+safe_id+'"]').html(safe_html);

        this.redrawSafeCharts();
    };

    this.redrawSafeCharts = function() {
        $('.send-timer').easyPieChart({
            easing: 'easeOutBounce',
            size: 32,
            lineWidth: 9,
            lineCap: 'butt',
            barColor: '#f0ad4e',
            onStep: function (from, to, percent) {
                $(this.el).find('.percent').text(Math.round(percent));
            }
        });
        $('.receive-timer').easyPieChart({
            easing: 'easeOutBounce',
            size: 32,
            lineWidth: 9,
            lineCap: 'butt',
            barColor: '#5bc0de',
            onStep: function (from, to, percent) {
                $(this.el).find('.percent').text(Math.round(percent));
            }
        });
    };

    this.subscribeToNonBackendEvents = function() {
        this.backend.subscribe("update_safe_count", function() {
            $app.updateSafeCarousel();
            $app.updateSafeCounters();
        });
        this.backend.subscribe("update_balance", function() {
            $app.updateBalanceCounters();
        });
        this.backend.subscribe("app_settings_updated", function() {
            $app.backend.contacts = $app.backend.applicationSettings.contacts;
        });
    };

    this.updateContactBook = function() {
        // Wait for settings to load
        if (!this.backend.contacts) {
            setTimeout(function() {
                $app.updateContactBook();
            }, 500);
            return;
        }

        // Initializing
        var contacts = this.backend.contacts;
        var insertion = $('.contacts_contact_insertion');
        insertion.html(''); // remove all

        for(var i in contacts) {
            var c = contacts[i];
            var html = $('.contacts_contact_template').html();

            var aliases = '';
            for(var j in c.aliases) aliases += '@' + c.aliases[j] + ' ';

            html = html.replace(/{{ contact.id }}/g, i);
            html = html.replace(/{{ contact.name }}/g, c.name);
            html = html.replace(/{{ contact.city }}/g, c.location.city);
            html = html.replace(/{{ contact.country }}/g, c.location.country);
            html = html.replace(/{{ contact.account }}/g, c.account);
            html = html.replace(/{{ contact.aliases }}/g, aliases);
            html = html.replace(/{{ contact.contactField }}/g, (Object.size(c.contactFields)>0) ? c.contactFields[Object.keys(c.contactFields)[0]] : '');
            html = html.replace(/{{ contact.note }}/g, c.note);
            html = html.replace(/{{ contact.groups }}/g, c.groups.join(', '));

            insertion.append(html);
        }
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
            if ($app.screens[screen] && $app.screens[screen].init) {
                $app.screens[screen].init();
            }
        }

        $app.backend.updateScreen(screen);
    };
    this.onScreenHide = function(screen) {
        if ($app.screens[screen] && $app.screens[screen].hide) {
            $app.screens[screen].hide();
        }
    };
    this.onScreenShow = function(screen) {
        if ($app.screens[screen] && $app.screens[screen].show) {
            $app.screens[screen].show();
        }
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
        if (name && screen) {
            $('.'+name+'_screen').hide();
            $('.'+name+'_'+screen).show();
        }
    };

    this.getRandomInt = function(min, max) {
        return Math.floor(Math.random() * (max - min + 1) + min);
    };
    this.getRandomObjectProperty = function(obj) {
        var keys = Object.keys(obj)
        return obj[keys[ keys.length * Math.random() << 0 ]];
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

    this.countries = [{"name":"Afghanistan","alpha-2":"AF","alpha-3":"AFG","country-code":"004","iso_3166-2":"ISO 3166-2:AF","region-code":"142","sub-region-code":"034"},{"name":"Åland Islands","alpha-2":"AX","alpha-3":"ALA","country-code":"248","iso_3166-2":"ISO 3166-2:AX","region-code":"150","sub-region-code":"154"},{"name":"Albania","alpha-2":"AL","alpha-3":"ALB","country-code":"008","iso_3166-2":"ISO 3166-2:AL","region-code":"150","sub-region-code":"039"},{"name":"Algeria","alpha-2":"DZ","alpha-3":"DZA","country-code":"012","iso_3166-2":"ISO 3166-2:DZ","region-code":"002","sub-region-code":"015"},{"name":"American Samoa","alpha-2":"AS","alpha-3":"ASM","country-code":"016","iso_3166-2":"ISO 3166-2:AS","region-code":"009","sub-region-code":"061"},{"name":"Andorra","alpha-2":"AD","alpha-3":"AND","country-code":"020","iso_3166-2":"ISO 3166-2:AD","region-code":"150","sub-region-code":"039"},{"name":"Angola","alpha-2":"AO","alpha-3":"AGO","country-code":"024","iso_3166-2":"ISO 3166-2:AO","region-code":"002","sub-region-code":"017"},{"name":"Anguilla","alpha-2":"AI","alpha-3":"AIA","country-code":"660","iso_3166-2":"ISO 3166-2:AI","region-code":"019","sub-region-code":"029"},{"name":"Antarctica","alpha-2":"AQ","alpha-3":"ATA","country-code":"010","iso_3166-2":"ISO 3166-2:AQ"},{"name":"Antigua and Barbuda","alpha-2":"AG","alpha-3":"ATG","country-code":"028","iso_3166-2":"ISO 3166-2:AG","region-code":"019","sub-region-code":"029"},{"name":"Argentina","alpha-2":"AR","alpha-3":"ARG","country-code":"032","iso_3166-2":"ISO 3166-2:AR","region-code":"019","sub-region-code":"005"},{"name":"Armenia","alpha-2":"AM","alpha-3":"ARM","country-code":"051","iso_3166-2":"ISO 3166-2:AM","region-code":"142","sub-region-code":"145"},{"name":"Aruba","alpha-2":"AW","alpha-3":"ABW","country-code":"533","iso_3166-2":"ISO 3166-2:AW","region-code":"019","sub-region-code":"029"},{"name":"Australia","alpha-2":"AU","alpha-3":"AUS","country-code":"036","iso_3166-2":"ISO 3166-2:AU","region-code":"009","sub-region-code":"053"},{"name":"Austria","alpha-2":"AT","alpha-3":"AUT","country-code":"040","iso_3166-2":"ISO 3166-2:AT","region-code":"150","sub-region-code":"155"},{"name":"Azerbaijan","alpha-2":"AZ","alpha-3":"AZE","country-code":"031","iso_3166-2":"ISO 3166-2:AZ","region-code":"142","sub-region-code":"145"},{"name":"Bahamas","alpha-2":"BS","alpha-3":"BHS","country-code":"044","iso_3166-2":"ISO 3166-2:BS","region-code":"019","sub-region-code":"029"},{"name":"Bahrain","alpha-2":"BH","alpha-3":"BHR","country-code":"048","iso_3166-2":"ISO 3166-2:BH","region-code":"142","sub-region-code":"145"},{"name":"Bangladesh","alpha-2":"BD","alpha-3":"BGD","country-code":"050","iso_3166-2":"ISO 3166-2:BD","region-code":"142","sub-region-code":"034"},{"name":"Barbados","alpha-2":"BB","alpha-3":"BRB","country-code":"052","iso_3166-2":"ISO 3166-2:BB","region-code":"019","sub-region-code":"029"},{"name":"Belarus","alpha-2":"BY","alpha-3":"BLR","country-code":"112","iso_3166-2":"ISO 3166-2:BY","region-code":"150","sub-region-code":"151"},{"name":"Belgium","alpha-2":"BE","alpha-3":"BEL","country-code":"056","iso_3166-2":"ISO 3166-2:BE","region-code":"150","sub-region-code":"155"},{"name":"Belize","alpha-2":"BZ","alpha-3":"BLZ","country-code":"084","iso_3166-2":"ISO 3166-2:BZ","region-code":"019","sub-region-code":"013"},{"name":"Benin","alpha-2":"BJ","alpha-3":"BEN","country-code":"204","iso_3166-2":"ISO 3166-2:BJ","region-code":"002","sub-region-code":"011"},{"name":"Bermuda","alpha-2":"BM","alpha-3":"BMU","country-code":"060","iso_3166-2":"ISO 3166-2:BM","region-code":"019","sub-region-code":"021"},{"name":"Bhutan","alpha-2":"BT","alpha-3":"BTN","country-code":"064","iso_3166-2":"ISO 3166-2:BT","region-code":"142","sub-region-code":"034"},{"name":"Bolivia, Plurinational State of","alpha-2":"BO","alpha-3":"BOL","country-code":"068","iso_3166-2":"ISO 3166-2:BO","region-code":"019","sub-region-code":"005"},{"name":"Bonaire, Sint Eustatius and Saba","alpha-2":"BQ","alpha-3":"BES","country-code":"535","iso_3166-2":"ISO 3166-2:BQ","region-code":"019","sub-region-code":"029"},{"name":"Bosnia and Herzegovina","alpha-2":"BA","alpha-3":"BIH","country-code":"070","iso_3166-2":"ISO 3166-2:BA","region-code":"150","sub-region-code":"039"},{"name":"Botswana","alpha-2":"BW","alpha-3":"BWA","country-code":"072","iso_3166-2":"ISO 3166-2:BW","region-code":"002","sub-region-code":"018"},{"name":"Bouvet Island","alpha-2":"BV","alpha-3":"BVT","country-code":"074","iso_3166-2":"ISO 3166-2:BV"},{"name":"Brazil","alpha-2":"BR","alpha-3":"BRA","country-code":"076","iso_3166-2":"ISO 3166-2:BR","region-code":"019","sub-region-code":"005"},{"name":"British Indian Ocean Territory","alpha-2":"IO","alpha-3":"IOT","country-code":"086","iso_3166-2":"ISO 3166-2:IO"},{"name":"Brunei Darussalam","alpha-2":"BN","alpha-3":"BRN","country-code":"096","iso_3166-2":"ISO 3166-2:BN","region-code":"142","sub-region-code":"035"},{"name":"Bulgaria","alpha-2":"BG","alpha-3":"BGR","country-code":"100","iso_3166-2":"ISO 3166-2:BG","region-code":"150","sub-region-code":"151"},{"name":"Burkina Faso","alpha-2":"BF","alpha-3":"BFA","country-code":"854","iso_3166-2":"ISO 3166-2:BF","region-code":"002","sub-region-code":"011"},{"name":"Burundi","alpha-2":"BI","alpha-3":"BDI","country-code":"108","iso_3166-2":"ISO 3166-2:BI","region-code":"002","sub-region-code":"014"},{"name":"Cambodia","alpha-2":"KH","alpha-3":"KHM","country-code":"116","iso_3166-2":"ISO 3166-2:KH","region-code":"142","sub-region-code":"035"},{"name":"Cameroon","alpha-2":"CM","alpha-3":"CMR","country-code":"120","iso_3166-2":"ISO 3166-2:CM","region-code":"002","sub-region-code":"017"},{"name":"Canada","alpha-2":"CA","alpha-3":"CAN","country-code":"124","iso_3166-2":"ISO 3166-2:CA","region-code":"019","sub-region-code":"021"},{"name":"Cape Verde","alpha-2":"CV","alpha-3":"CPV","country-code":"132","iso_3166-2":"ISO 3166-2:CV","region-code":"002","sub-region-code":"011"},{"name":"Cayman Islands","alpha-2":"KY","alpha-3":"CYM","country-code":"136","iso_3166-2":"ISO 3166-2:KY","region-code":"019","sub-region-code":"029"},{"name":"Central African Republic","alpha-2":"CF","alpha-3":"CAF","country-code":"140","iso_3166-2":"ISO 3166-2:CF","region-code":"002","sub-region-code":"017"},{"name":"Chad","alpha-2":"TD","alpha-3":"TCD","country-code":"148","iso_3166-2":"ISO 3166-2:TD","region-code":"002","sub-region-code":"017"},{"name":"Chile","alpha-2":"CL","alpha-3":"CHL","country-code":"152","iso_3166-2":"ISO 3166-2:CL","region-code":"019","sub-region-code":"005"},{"name":"China","alpha-2":"CN","alpha-3":"CHN","country-code":"156","iso_3166-2":"ISO 3166-2:CN","region-code":"142","sub-region-code":"030"},{"name":"Christmas Island","alpha-2":"CX","alpha-3":"CXR","country-code":"162","iso_3166-2":"ISO 3166-2:CX"},{"name":"Cocos (Keeling) Islands","alpha-2":"CC","alpha-3":"CCK","country-code":"166","iso_3166-2":"ISO 3166-2:CC"},{"name":"Colombia","alpha-2":"CO","alpha-3":"COL","country-code":"170","iso_3166-2":"ISO 3166-2:CO","region-code":"019","sub-region-code":"005"},{"name":"Comoros","alpha-2":"KM","alpha-3":"COM","country-code":"174","iso_3166-2":"ISO 3166-2:KM","region-code":"002","sub-region-code":"014"},{"name":"Congo","alpha-2":"CG","alpha-3":"COG","country-code":"178","iso_3166-2":"ISO 3166-2:CG","region-code":"002","sub-region-code":"017"},{"name":"Congo, the Democratic Republic of the","alpha-2":"CD","alpha-3":"COD","country-code":"180","iso_3166-2":"ISO 3166-2:CD","region-code":"002","sub-region-code":"017"},{"name":"Cook Islands","alpha-2":"CK","alpha-3":"COK","country-code":"184","iso_3166-2":"ISO 3166-2:CK","region-code":"009","sub-region-code":"061"},{"name":"Costa Rica","alpha-2":"CR","alpha-3":"CRI","country-code":"188","iso_3166-2":"ISO 3166-2:CR","region-code":"019","sub-region-code":"013"},{"name":"Côte d'Ivoire","alpha-2":"CI","alpha-3":"CIV","country-code":"384","iso_3166-2":"ISO 3166-2:CI","region-code":"002","sub-region-code":"011"},{"name":"Croatia","alpha-2":"HR","alpha-3":"HRV","country-code":"191","iso_3166-2":"ISO 3166-2:HR","region-code":"150","sub-region-code":"039"},{"name":"Cuba","alpha-2":"CU","alpha-3":"CUB","country-code":"192","iso_3166-2":"ISO 3166-2:CU","region-code":"019","sub-region-code":"029"},{"name":"Curaçao","alpha-2":"CW","alpha-3":"CUW","country-code":"531","iso_3166-2":"ISO 3166-2:CW","region-code":"019","sub-region-code":"029"},{"name":"Cyprus","alpha-2":"CY","alpha-3":"CYP","country-code":"196","iso_3166-2":"ISO 3166-2:CY","region-code":"142","sub-region-code":"145"},{"name":"Czech Republic","alpha-2":"CZ","alpha-3":"CZE","country-code":"203","iso_3166-2":"ISO 3166-2:CZ","region-code":"150","sub-region-code":"151"},{"name":"Denmark","alpha-2":"DK","alpha-3":"DNK","country-code":"208","iso_3166-2":"ISO 3166-2:DK","region-code":"150","sub-region-code":"154"},{"name":"Djibouti","alpha-2":"DJ","alpha-3":"DJI","country-code":"262","iso_3166-2":"ISO 3166-2:DJ","region-code":"002","sub-region-code":"014"},{"name":"Dominica","alpha-2":"DM","alpha-3":"DMA","country-code":"212","iso_3166-2":"ISO 3166-2:DM","region-code":"019","sub-region-code":"029"},{"name":"Dominican Republic","alpha-2":"DO","alpha-3":"DOM","country-code":"214","iso_3166-2":"ISO 3166-2:DO","region-code":"019","sub-region-code":"029"},{"name":"Ecuador","alpha-2":"EC","alpha-3":"ECU","country-code":"218","iso_3166-2":"ISO 3166-2:EC","region-code":"019","sub-region-code":"005"},{"name":"Egypt","alpha-2":"EG","alpha-3":"EGY","country-code":"818","iso_3166-2":"ISO 3166-2:EG","region-code":"002","sub-region-code":"015"},{"name":"El Salvador","alpha-2":"SV","alpha-3":"SLV","country-code":"222","iso_3166-2":"ISO 3166-2:SV","region-code":"019","sub-region-code":"013"},{"name":"Equatorial Guinea","alpha-2":"GQ","alpha-3":"GNQ","country-code":"226","iso_3166-2":"ISO 3166-2:GQ","region-code":"002","sub-region-code":"017"},{"name":"Eritrea","alpha-2":"ER","alpha-3":"ERI","country-code":"232","iso_3166-2":"ISO 3166-2:ER","region-code":"002","sub-region-code":"014"},{"name":"Estonia","alpha-2":"EE","alpha-3":"EST","country-code":"233","iso_3166-2":"ISO 3166-2:EE","region-code":"150","sub-region-code":"154"},{"name":"Ethiopia","alpha-2":"ET","alpha-3":"ETH","country-code":"231","iso_3166-2":"ISO 3166-2:ET","region-code":"002","sub-region-code":"014"},{"name":"Falkland Islands (Malvinas)","alpha-2":"FK","alpha-3":"FLK","country-code":"238","iso_3166-2":"ISO 3166-2:FK","region-code":"019","sub-region-code":"005"},{"name":"Faroe Islands","alpha-2":"FO","alpha-3":"FRO","country-code":"234","iso_3166-2":"ISO 3166-2:FO","region-code":"150","sub-region-code":"154"},{"name":"Fiji","alpha-2":"FJ","alpha-3":"FJI","country-code":"242","iso_3166-2":"ISO 3166-2:FJ","region-code":"009","sub-region-code":"054"},{"name":"Finland","alpha-2":"FI","alpha-3":"FIN","country-code":"246","iso_3166-2":"ISO 3166-2:FI","region-code":"150","sub-region-code":"154"},{"name":"France","alpha-2":"FR","alpha-3":"FRA","country-code":"250","iso_3166-2":"ISO 3166-2:FR","region-code":"150","sub-region-code":"155"},{"name":"French Guiana","alpha-2":"GF","alpha-3":"GUF","country-code":"254","iso_3166-2":"ISO 3166-2:GF","region-code":"019","sub-region-code":"005"},{"name":"French Polynesia","alpha-2":"PF","alpha-3":"PYF","country-code":"258","iso_3166-2":"ISO 3166-2:PF","region-code":"009","sub-region-code":"061"},{"name":"French Southern Territories","alpha-2":"TF","alpha-3":"ATF","country-code":"260","iso_3166-2":"ISO 3166-2:TF"},{"name":"Gabon","alpha-2":"GA","alpha-3":"GAB","country-code":"266","iso_3166-2":"ISO 3166-2:GA","region-code":"002","sub-region-code":"017"},{"name":"Gambia","alpha-2":"GM","alpha-3":"GMB","country-code":"270","iso_3166-2":"ISO 3166-2:GM","region-code":"002","sub-region-code":"011"},{"name":"Georgia","alpha-2":"GE","alpha-3":"GEO","country-code":"268","iso_3166-2":"ISO 3166-2:GE","region-code":"142","sub-region-code":"145"},{"name":"Germany","alpha-2":"DE","alpha-3":"DEU","country-code":"276","iso_3166-2":"ISO 3166-2:DE","region-code":"150","sub-region-code":"155"},{"name":"Ghana","alpha-2":"GH","alpha-3":"GHA","country-code":"288","iso_3166-2":"ISO 3166-2:GH","region-code":"002","sub-region-code":"011"},{"name":"Gibraltar","alpha-2":"GI","alpha-3":"GIB","country-code":"292","iso_3166-2":"ISO 3166-2:GI","region-code":"150","sub-region-code":"039"},{"name":"Greece","alpha-2":"GR","alpha-3":"GRC","country-code":"300","iso_3166-2":"ISO 3166-2:GR","region-code":"150","sub-region-code":"039"},{"name":"Greenland","alpha-2":"GL","alpha-3":"GRL","country-code":"304","iso_3166-2":"ISO 3166-2:GL","region-code":"019","sub-region-code":"021"},{"name":"Grenada","alpha-2":"GD","alpha-3":"GRD","country-code":"308","iso_3166-2":"ISO 3166-2:GD","region-code":"019","sub-region-code":"029"},{"name":"Guadeloupe","alpha-2":"GP","alpha-3":"GLP","country-code":"312","iso_3166-2":"ISO 3166-2:GP","region-code":"019","sub-region-code":"029"},{"name":"Guam","alpha-2":"GU","alpha-3":"GUM","country-code":"316","iso_3166-2":"ISO 3166-2:GU","region-code":"009","sub-region-code":"057"},{"name":"Guatemala","alpha-2":"GT","alpha-3":"GTM","country-code":"320","iso_3166-2":"ISO 3166-2:GT","region-code":"019","sub-region-code":"013"},{"name":"Guernsey","alpha-2":"GG","alpha-3":"GGY","country-code":"831","iso_3166-2":"ISO 3166-2:GG","region-code":"150","sub-region-code":"154"},{"name":"Guinea","alpha-2":"GN","alpha-3":"GIN","country-code":"324","iso_3166-2":"ISO 3166-2:GN","region-code":"002","sub-region-code":"011"},{"name":"Guinea-Bissau","alpha-2":"GW","alpha-3":"GNB","country-code":"624","iso_3166-2":"ISO 3166-2:GW","region-code":"002","sub-region-code":"011"},{"name":"Guyana","alpha-2":"GY","alpha-3":"GUY","country-code":"328","iso_3166-2":"ISO 3166-2:GY","region-code":"019","sub-region-code":"005"},{"name":"Haiti","alpha-2":"HT","alpha-3":"HTI","country-code":"332","iso_3166-2":"ISO 3166-2:HT","region-code":"019","sub-region-code":"029"},{"name":"Heard Island and McDonald Islands","alpha-2":"HM","alpha-3":"HMD","country-code":"334","iso_3166-2":"ISO 3166-2:HM"},{"name":"Holy See (Vatican City State)","alpha-2":"VA","alpha-3":"VAT","country-code":"336","iso_3166-2":"ISO 3166-2:VA","region-code":"150","sub-region-code":"039"},{"name":"Honduras","alpha-2":"HN","alpha-3":"HND","country-code":"340","iso_3166-2":"ISO 3166-2:HN","region-code":"019","sub-region-code":"013"},{"name":"Hong Kong","alpha-2":"HK","alpha-3":"HKG","country-code":"344","iso_3166-2":"ISO 3166-2:HK","region-code":"142","sub-region-code":"030"},{"name":"Hungary","alpha-2":"HU","alpha-3":"HUN","country-code":"348","iso_3166-2":"ISO 3166-2:HU","region-code":"150","sub-region-code":"151"},{"name":"Iceland","alpha-2":"IS","alpha-3":"ISL","country-code":"352","iso_3166-2":"ISO 3166-2:IS","region-code":"150","sub-region-code":"154"},{"name":"India","alpha-2":"IN","alpha-3":"IND","country-code":"356","iso_3166-2":"ISO 3166-2:IN","region-code":"142","sub-region-code":"034"},{"name":"Indonesia","alpha-2":"ID","alpha-3":"IDN","country-code":"360","iso_3166-2":"ISO 3166-2:ID","region-code":"142","sub-region-code":"035"},{"name":"Iran, Islamic Republic of","alpha-2":"IR","alpha-3":"IRN","country-code":"364","iso_3166-2":"ISO 3166-2:IR","region-code":"142","sub-region-code":"034"},{"name":"Iraq","alpha-2":"IQ","alpha-3":"IRQ","country-code":"368","iso_3166-2":"ISO 3166-2:IQ","region-code":"142","sub-region-code":"145"},{"name":"Ireland","alpha-2":"IE","alpha-3":"IRL","country-code":"372","iso_3166-2":"ISO 3166-2:IE","region-code":"150","sub-region-code":"154"},{"name":"Isle of Man","alpha-2":"IM","alpha-3":"IMN","country-code":"833","iso_3166-2":"ISO 3166-2:IM","region-code":"150","sub-region-code":"154"},{"name":"Israel","alpha-2":"IL","alpha-3":"ISR","country-code":"376","iso_3166-2":"ISO 3166-2:IL","region-code":"142","sub-region-code":"145"},{"name":"Italy","alpha-2":"IT","alpha-3":"ITA","country-code":"380","iso_3166-2":"ISO 3166-2:IT","region-code":"150","sub-region-code":"039"},{"name":"Jamaica","alpha-2":"JM","alpha-3":"JAM","country-code":"388","iso_3166-2":"ISO 3166-2:JM","region-code":"019","sub-region-code":"029"},{"name":"Japan","alpha-2":"JP","alpha-3":"JPN","country-code":"392","iso_3166-2":"ISO 3166-2:JP","region-code":"142","sub-region-code":"030"},{"name":"Jersey","alpha-2":"JE","alpha-3":"JEY","country-code":"832","iso_3166-2":"ISO 3166-2:JE","region-code":"150","sub-region-code":"154"},{"name":"Jordan","alpha-2":"JO","alpha-3":"JOR","country-code":"400","iso_3166-2":"ISO 3166-2:JO","region-code":"142","sub-region-code":"145"},{"name":"Kazakhstan","alpha-2":"KZ","alpha-3":"KAZ","country-code":"398","iso_3166-2":"ISO 3166-2:KZ","region-code":"142","sub-region-code":"143"},{"name":"Kenya","alpha-2":"KE","alpha-3":"KEN","country-code":"404","iso_3166-2":"ISO 3166-2:KE","region-code":"002","sub-region-code":"014"},{"name":"Kiribati","alpha-2":"KI","alpha-3":"KIR","country-code":"296","iso_3166-2":"ISO 3166-2:KI","region-code":"009","sub-region-code":"057"},{"name":"Korea, Democratic People's Republic of","alpha-2":"KP","alpha-3":"PRK","country-code":"408","iso_3166-2":"ISO 3166-2:KP","region-code":"142","sub-region-code":"030"},{"name":"Korea, Republic of","alpha-2":"KR","alpha-3":"KOR","country-code":"410","iso_3166-2":"ISO 3166-2:KR","region-code":"142","sub-region-code":"030"},{"name":"Kuwait","alpha-2":"KW","alpha-3":"KWT","country-code":"414","iso_3166-2":"ISO 3166-2:KW","region-code":"142","sub-region-code":"145"},{"name":"Kyrgyzstan","alpha-2":"KG","alpha-3":"KGZ","country-code":"417","iso_3166-2":"ISO 3166-2:KG","region-code":"142","sub-region-code":"143"},{"name":"Lao People's Democratic Republic","alpha-2":"LA","alpha-3":"LAO","country-code":"418","iso_3166-2":"ISO 3166-2:LA","region-code":"142","sub-region-code":"035"},{"name":"Latvia","alpha-2":"LV","alpha-3":"LVA","country-code":"428","iso_3166-2":"ISO 3166-2:LV","region-code":"150","sub-region-code":"154"},{"name":"Lebanon","alpha-2":"LB","alpha-3":"LBN","country-code":"422","iso_3166-2":"ISO 3166-2:LB","region-code":"142","sub-region-code":"145"},{"name":"Lesotho","alpha-2":"LS","alpha-3":"LSO","country-code":"426","iso_3166-2":"ISO 3166-2:LS","region-code":"002","sub-region-code":"018"},{"name":"Liberia","alpha-2":"LR","alpha-3":"LBR","country-code":"430","iso_3166-2":"ISO 3166-2:LR","region-code":"002","sub-region-code":"011"},{"name":"Libya","alpha-2":"LY","alpha-3":"LBY","country-code":"434","iso_3166-2":"ISO 3166-2:LY","region-code":"002","sub-region-code":"015"},{"name":"Liechtenstein","alpha-2":"LI","alpha-3":"LIE","country-code":"438","iso_3166-2":"ISO 3166-2:LI","region-code":"150","sub-region-code":"155"},{"name":"Lithuania","alpha-2":"LT","alpha-3":"LTU","country-code":"440","iso_3166-2":"ISO 3166-2:LT","region-code":"150","sub-region-code":"154"},{"name":"Luxembourg","alpha-2":"LU","alpha-3":"LUX","country-code":"442","iso_3166-2":"ISO 3166-2:LU","region-code":"150","sub-region-code":"155"},{"name":"Macao","alpha-2":"MO","alpha-3":"MAC","country-code":"446","iso_3166-2":"ISO 3166-2:MO","region-code":"142","sub-region-code":"030"},{"name":"Macedonia, the former Yugoslav Republic of","alpha-2":"MK","alpha-3":"MKD","country-code":"807","iso_3166-2":"ISO 3166-2:MK","region-code":"150","sub-region-code":"039"},{"name":"Madagascar","alpha-2":"MG","alpha-3":"MDG","country-code":"450","iso_3166-2":"ISO 3166-2:MG","region-code":"002","sub-region-code":"014"},{"name":"Malawi","alpha-2":"MW","alpha-3":"MWI","country-code":"454","iso_3166-2":"ISO 3166-2:MW","region-code":"002","sub-region-code":"014"},{"name":"Malaysia","alpha-2":"MY","alpha-3":"MYS","country-code":"458","iso_3166-2":"ISO 3166-2:MY","region-code":"142","sub-region-code":"035"},{"name":"Maldives","alpha-2":"MV","alpha-3":"MDV","country-code":"462","iso_3166-2":"ISO 3166-2:MV","region-code":"142","sub-region-code":"034"},{"name":"Mali","alpha-2":"ML","alpha-3":"MLI","country-code":"466","iso_3166-2":"ISO 3166-2:ML","region-code":"002","sub-region-code":"011"},{"name":"Malta","alpha-2":"MT","alpha-3":"MLT","country-code":"470","iso_3166-2":"ISO 3166-2:MT","region-code":"150","sub-region-code":"039"},{"name":"Marshall Islands","alpha-2":"MH","alpha-3":"MHL","country-code":"584","iso_3166-2":"ISO 3166-2:MH","region-code":"009","sub-region-code":"057"},{"name":"Martinique","alpha-2":"MQ","alpha-3":"MTQ","country-code":"474","iso_3166-2":"ISO 3166-2:MQ","region-code":"019","sub-region-code":"029"},{"name":"Mauritania","alpha-2":"MR","alpha-3":"MRT","country-code":"478","iso_3166-2":"ISO 3166-2:MR","region-code":"002","sub-region-code":"011"},{"name":"Mauritius","alpha-2":"MU","alpha-3":"MUS","country-code":"480","iso_3166-2":"ISO 3166-2:MU","region-code":"002","sub-region-code":"014"},{"name":"Mayotte","alpha-2":"YT","alpha-3":"MYT","country-code":"175","iso_3166-2":"ISO 3166-2:YT","region-code":"002","sub-region-code":"014"},{"name":"Mexico","alpha-2":"MX","alpha-3":"MEX","country-code":"484","iso_3166-2":"ISO 3166-2:MX","region-code":"019","sub-region-code":"013"},{"name":"Micronesia, Federated States of","alpha-2":"FM","alpha-3":"FSM","country-code":"583","iso_3166-2":"ISO 3166-2:FM","region-code":"009","sub-region-code":"057"},{"name":"Moldova, Republic of","alpha-2":"MD","alpha-3":"MDA","country-code":"498","iso_3166-2":"ISO 3166-2:MD","region-code":"150","sub-region-code":"151"},{"name":"Monaco","alpha-2":"MC","alpha-3":"MCO","country-code":"492","iso_3166-2":"ISO 3166-2:MC","region-code":"150","sub-region-code":"155"},{"name":"Mongolia","alpha-2":"MN","alpha-3":"MNG","country-code":"496","iso_3166-2":"ISO 3166-2:MN","region-code":"142","sub-region-code":"030"},{"name":"Montenegro","alpha-2":"ME","alpha-3":"MNE","country-code":"499","iso_3166-2":"ISO 3166-2:ME","region-code":"150","sub-region-code":"039"},{"name":"Montserrat","alpha-2":"MS","alpha-3":"MSR","country-code":"500","iso_3166-2":"ISO 3166-2:MS","region-code":"019","sub-region-code":"029"},{"name":"Morocco","alpha-2":"MA","alpha-3":"MAR","country-code":"504","iso_3166-2":"ISO 3166-2:MA","region-code":"002","sub-region-code":"015"},{"name":"Mozambique","alpha-2":"MZ","alpha-3":"MOZ","country-code":"508","iso_3166-2":"ISO 3166-2:MZ","region-code":"002","sub-region-code":"014"},{"name":"Myanmar","alpha-2":"MM","alpha-3":"MMR","country-code":"104","iso_3166-2":"ISO 3166-2:MM","region-code":"142","sub-region-code":"035"},{"name":"Namibia","alpha-2":"NA","alpha-3":"NAM","country-code":"516","iso_3166-2":"ISO 3166-2:NA","region-code":"002","sub-region-code":"018"},{"name":"Nauru","alpha-2":"NR","alpha-3":"NRU","country-code":"520","iso_3166-2":"ISO 3166-2:NR","region-code":"009","sub-region-code":"057"},{"name":"Nepal","alpha-2":"NP","alpha-3":"NPL","country-code":"524","iso_3166-2":"ISO 3166-2:NP","region-code":"142","sub-region-code":"034"},{"name":"Netherlands","alpha-2":"NL","alpha-3":"NLD","country-code":"528","iso_3166-2":"ISO 3166-2:NL","region-code":"150","sub-region-code":"155"},{"name":"New Caledonia","alpha-2":"NC","alpha-3":"NCL","country-code":"540","iso_3166-2":"ISO 3166-2:NC","region-code":"009","sub-region-code":"054"},{"name":"New Zealand","alpha-2":"NZ","alpha-3":"NZL","country-code":"554","iso_3166-2":"ISO 3166-2:NZ","region-code":"009","sub-region-code":"053"},{"name":"Nicaragua","alpha-2":"NI","alpha-3":"NIC","country-code":"558","iso_3166-2":"ISO 3166-2:NI","region-code":"019","sub-region-code":"013"},{"name":"Niger","alpha-2":"NE","alpha-3":"NER","country-code":"562","iso_3166-2":"ISO 3166-2:NE","region-code":"002","sub-region-code":"011"},{"name":"Nigeria","alpha-2":"NG","alpha-3":"NGA","country-code":"566","iso_3166-2":"ISO 3166-2:NG","region-code":"002","sub-region-code":"011"},{"name":"Niue","alpha-2":"NU","alpha-3":"NIU","country-code":"570","iso_3166-2":"ISO 3166-2:NU","region-code":"009","sub-region-code":"061"},{"name":"Norfolk Island","alpha-2":"NF","alpha-3":"NFK","country-code":"574","iso_3166-2":"ISO 3166-2:NF","region-code":"009","sub-region-code":"053"},{"name":"Northern Mariana Islands","alpha-2":"MP","alpha-3":"MNP","country-code":"580","iso_3166-2":"ISO 3166-2:MP","region-code":"009","sub-region-code":"057"},{"name":"Norway","alpha-2":"NO","alpha-3":"NOR","country-code":"578","iso_3166-2":"ISO 3166-2:NO","region-code":"150","sub-region-code":"154"},{"name":"Oman","alpha-2":"OM","alpha-3":"OMN","country-code":"512","iso_3166-2":"ISO 3166-2:OM","region-code":"142","sub-region-code":"145"},{"name":"Pakistan","alpha-2":"PK","alpha-3":"PAK","country-code":"586","iso_3166-2":"ISO 3166-2:PK","region-code":"142","sub-region-code":"034"},{"name":"Palau","alpha-2":"PW","alpha-3":"PLW","country-code":"585","iso_3166-2":"ISO 3166-2:PW","region-code":"009","sub-region-code":"057"},{"name":"Palestine, State of","alpha-2":"PS","alpha-3":"PSE","country-code":"275","iso_3166-2":"ISO 3166-2:PS","region-code":"142","sub-region-code":"145"},{"name":"Panama","alpha-2":"PA","alpha-3":"PAN","country-code":"591","iso_3166-2":"ISO 3166-2:PA","region-code":"019","sub-region-code":"013"},{"name":"Papua New Guinea","alpha-2":"PG","alpha-3":"PNG","country-code":"598","iso_3166-2":"ISO 3166-2:PG","region-code":"009","sub-region-code":"054"},{"name":"Paraguay","alpha-2":"PY","alpha-3":"PRY","country-code":"600","iso_3166-2":"ISO 3166-2:PY","region-code":"019","sub-region-code":"005"},{"name":"Peru","alpha-2":"PE","alpha-3":"PER","country-code":"604","iso_3166-2":"ISO 3166-2:PE","region-code":"019","sub-region-code":"005"},{"name":"Philippines","alpha-2":"PH","alpha-3":"PHL","country-code":"608","iso_3166-2":"ISO 3166-2:PH","region-code":"142","sub-region-code":"035"},{"name":"Pitcairn","alpha-2":"PN","alpha-3":"PCN","country-code":"612","iso_3166-2":"ISO 3166-2:PN","region-code":"009","sub-region-code":"061"},{"name":"Poland","alpha-2":"PL","alpha-3":"POL","country-code":"616","iso_3166-2":"ISO 3166-2:PL","region-code":"150","sub-region-code":"151"},{"name":"Portugal","alpha-2":"PT","alpha-3":"PRT","country-code":"620","iso_3166-2":"ISO 3166-2:PT","region-code":"150","sub-region-code":"039"},{"name":"Puerto Rico","alpha-2":"PR","alpha-3":"PRI","country-code":"630","iso_3166-2":"ISO 3166-2:PR","region-code":"019","sub-region-code":"029"},{"name":"Qatar","alpha-2":"QA","alpha-3":"QAT","country-code":"634","iso_3166-2":"ISO 3166-2:QA","region-code":"142","sub-region-code":"145"},{"name":"Réunion","alpha-2":"RE","alpha-3":"REU","country-code":"638","iso_3166-2":"ISO 3166-2:RE","region-code":"002","sub-region-code":"014"},{"name":"Romania","alpha-2":"RO","alpha-3":"ROU","country-code":"642","iso_3166-2":"ISO 3166-2:RO","region-code":"150","sub-region-code":"151"},{"name":"Russian Federation","alpha-2":"RU","alpha-3":"RUS","country-code":"643","iso_3166-2":"ISO 3166-2:RU","region-code":"150","sub-region-code":"151"},{"name":"Rwanda","alpha-2":"RW","alpha-3":"RWA","country-code":"646","iso_3166-2":"ISO 3166-2:RW","region-code":"002","sub-region-code":"014"},{"name":"Saint Barthélemy","alpha-2":"BL","alpha-3":"BLM","country-code":"652","iso_3166-2":"ISO 3166-2:BL","region-code":"019","sub-region-code":"029"},{"name":"Saint Helena, Ascension and Tristan da Cunha","alpha-2":"SH","alpha-3":"SHN","country-code":"654","iso_3166-2":"ISO 3166-2:SH","region-code":"002","sub-region-code":"011"},{"name":"Saint Kitts and Nevis","alpha-2":"KN","alpha-3":"KNA","country-code":"659","iso_3166-2":"ISO 3166-2:KN","region-code":"019","sub-region-code":"029"},{"name":"Saint Lucia","alpha-2":"LC","alpha-3":"LCA","country-code":"662","iso_3166-2":"ISO 3166-2:LC","region-code":"019","sub-region-code":"029"},{"name":"Saint Martin (French part)","alpha-2":"MF","alpha-3":"MAF","country-code":"663","iso_3166-2":"ISO 3166-2:MF","region-code":"019","sub-region-code":"029"},{"name":"Saint Pierre and Miquelon","alpha-2":"PM","alpha-3":"SPM","country-code":"666","iso_3166-2":"ISO 3166-2:PM","region-code":"019","sub-region-code":"021"},{"name":"Saint Vincent and the Grenadines","alpha-2":"VC","alpha-3":"VCT","country-code":"670","iso_3166-2":"ISO 3166-2:VC","region-code":"019","sub-region-code":"029"},{"name":"Samoa","alpha-2":"WS","alpha-3":"WSM","country-code":"882","iso_3166-2":"ISO 3166-2:WS","region-code":"009","sub-region-code":"061"},{"name":"San Marino","alpha-2":"SM","alpha-3":"SMR","country-code":"674","iso_3166-2":"ISO 3166-2:SM","region-code":"150","sub-region-code":"039"},{"name":"Sao Tome and Principe","alpha-2":"ST","alpha-3":"STP","country-code":"678","iso_3166-2":"ISO 3166-2:ST","region-code":"002","sub-region-code":"017"},{"name":"Saudi Arabia","alpha-2":"SA","alpha-3":"SAU","country-code":"682","iso_3166-2":"ISO 3166-2:SA","region-code":"142","sub-region-code":"145"},{"name":"Senegal","alpha-2":"SN","alpha-3":"SEN","country-code":"686","iso_3166-2":"ISO 3166-2:SN","region-code":"002","sub-region-code":"011"},{"name":"Serbia","alpha-2":"RS","alpha-3":"SRB","country-code":"688","iso_3166-2":"ISO 3166-2:RS","region-code":"150","sub-region-code":"039"},{"name":"Seychelles","alpha-2":"SC","alpha-3":"SYC","country-code":"690","iso_3166-2":"ISO 3166-2:SC","region-code":"002","sub-region-code":"014"},{"name":"Sierra Leone","alpha-2":"SL","alpha-3":"SLE","country-code":"694","iso_3166-2":"ISO 3166-2:SL","region-code":"002","sub-region-code":"011"},{"name":"Singapore","alpha-2":"SG","alpha-3":"SGP","country-code":"702","iso_3166-2":"ISO 3166-2:SG","region-code":"142","sub-region-code":"035"},{"name":"Sint Maarten (Dutch part)","alpha-2":"SX","alpha-3":"SXM","country-code":"534","iso_3166-2":"ISO 3166-2:SX","region-code":"019","sub-region-code":"029"},{"name":"Slovakia","alpha-2":"SK","alpha-3":"SVK","country-code":"703","iso_3166-2":"ISO 3166-2:SK","region-code":"150","sub-region-code":"151"},{"name":"Slovenia","alpha-2":"SI","alpha-3":"SVN","country-code":"705","iso_3166-2":"ISO 3166-2:SI","region-code":"150","sub-region-code":"039"},{"name":"Solomon Islands","alpha-2":"SB","alpha-3":"SLB","country-code":"090","iso_3166-2":"ISO 3166-2:SB","region-code":"009","sub-region-code":"054"},{"name":"Somalia","alpha-2":"SO","alpha-3":"SOM","country-code":"706","iso_3166-2":"ISO 3166-2:SO","region-code":"002","sub-region-code":"014"},{"name":"South Africa","alpha-2":"ZA","alpha-3":"ZAF","country-code":"710","iso_3166-2":"ISO 3166-2:ZA","region-code":"002","sub-region-code":"018"},{"name":"South Georgia and the South Sandwich Islands","alpha-2":"GS","alpha-3":"SGS","country-code":"239","iso_3166-2":"ISO 3166-2:GS"},{"name":"South Sudan","alpha-2":"SS","alpha-3":"SSD","country-code":"728","iso_3166-2":"ISO 3166-2:SS","region-code":"002","sub-region-code":"014"},{"name":"Spain","alpha-2":"ES","alpha-3":"ESP","country-code":"724","iso_3166-2":"ISO 3166-2:ES","region-code":"150","sub-region-code":"039"},{"name":"Sri Lanka","alpha-2":"LK","alpha-3":"LKA","country-code":"144","iso_3166-2":"ISO 3166-2:LK","region-code":"142","sub-region-code":"034"},{"name":"Sudan","alpha-2":"SD","alpha-3":"SDN","country-code":"729","iso_3166-2":"ISO 3166-2:SD","region-code":"002","sub-region-code":"015"},{"name":"Suriname","alpha-2":"SR","alpha-3":"SUR","country-code":"740","iso_3166-2":"ISO 3166-2:SR","region-code":"019","sub-region-code":"005"},{"name":"Svalbard and Jan Mayen","alpha-2":"SJ","alpha-3":"SJM","country-code":"744","iso_3166-2":"ISO 3166-2:SJ","region-code":"150","sub-region-code":"154"},{"name":"Swaziland","alpha-2":"SZ","alpha-3":"SWZ","country-code":"748","iso_3166-2":"ISO 3166-2:SZ","region-code":"002","sub-region-code":"018"},{"name":"Sweden","alpha-2":"SE","alpha-3":"SWE","country-code":"752","iso_3166-2":"ISO 3166-2:SE","region-code":"150","sub-region-code":"154"},{"name":"Switzerland","alpha-2":"CH","alpha-3":"CHE","country-code":"756","iso_3166-2":"ISO 3166-2:CH","region-code":"150","sub-region-code":"155"},{"name":"Syrian Arab Republic","alpha-2":"SY","alpha-3":"SYR","country-code":"760","iso_3166-2":"ISO 3166-2:SY","region-code":"142","sub-region-code":"145"},{"name":"Taiwan, Province of China","alpha-2":"TW","alpha-3":"TWN","country-code":"158","iso_3166-2":"ISO 3166-2:TW","region-code":"142","sub-region-code":"030"},{"name":"Tajikistan","alpha-2":"TJ","alpha-3":"TJK","country-code":"762","iso_3166-2":"ISO 3166-2:TJ","region-code":"142","sub-region-code":"143"},{"name":"Tanzania, United Republic of","alpha-2":"TZ","alpha-3":"TZA","country-code":"834","iso_3166-2":"ISO 3166-2:TZ","region-code":"002","sub-region-code":"014"},{"name":"Thailand","alpha-2":"TH","alpha-3":"THA","country-code":"764","iso_3166-2":"ISO 3166-2:TH","region-code":"142","sub-region-code":"035"},{"name":"Timor-Leste","alpha-2":"TL","alpha-3":"TLS","country-code":"626","iso_3166-2":"ISO 3166-2:TL","region-code":"142","sub-region-code":"035"},{"name":"Togo","alpha-2":"TG","alpha-3":"TGO","country-code":"768","iso_3166-2":"ISO 3166-2:TG","region-code":"002","sub-region-code":"011"},{"name":"Tokelau","alpha-2":"TK","alpha-3":"TKL","country-code":"772","iso_3166-2":"ISO 3166-2:TK","region-code":"009","sub-region-code":"061"},{"name":"Tonga","alpha-2":"TO","alpha-3":"TON","country-code":"776","iso_3166-2":"ISO 3166-2:TO","region-code":"009","sub-region-code":"061"},{"name":"Trinidad and Tobago","alpha-2":"TT","alpha-3":"TTO","country-code":"780","iso_3166-2":"ISO 3166-2:TT","region-code":"019","sub-region-code":"029"},{"name":"Tunisia","alpha-2":"TN","alpha-3":"TUN","country-code":"788","iso_3166-2":"ISO 3166-2:TN","region-code":"002","sub-region-code":"015"},{"name":"Turkey","alpha-2":"TR","alpha-3":"TUR","country-code":"792","iso_3166-2":"ISO 3166-2:TR","region-code":"142","sub-region-code":"145"},{"name":"Turkmenistan","alpha-2":"TM","alpha-3":"TKM","country-code":"795","iso_3166-2":"ISO 3166-2:TM","region-code":"142","sub-region-code":"143"},{"name":"Turks and Caicos Islands","alpha-2":"TC","alpha-3":"TCA","country-code":"796","iso_3166-2":"ISO 3166-2:TC","region-code":"019","sub-region-code":"029"},{"name":"Tuvalu","alpha-2":"TV","alpha-3":"TUV","country-code":"798","iso_3166-2":"ISO 3166-2:TV","region-code":"009","sub-region-code":"061"},{"name":"Uganda","alpha-2":"UG","alpha-3":"UGA","country-code":"800","iso_3166-2":"ISO 3166-2:UG","region-code":"002","sub-region-code":"014"},{"name":"Ukraine","alpha-2":"UA","alpha-3":"UKR","country-code":"804","iso_3166-2":"ISO 3166-2:UA","region-code":"150","sub-region-code":"151"},{"name":"United Arab Emirates","alpha-2":"AE","alpha-3":"ARE","country-code":"784","iso_3166-2":"ISO 3166-2:AE","region-code":"142","sub-region-code":"145"},{"name":"United Kingdom","alpha-2":"GB","alpha-3":"GBR","country-code":"826","iso_3166-2":"ISO 3166-2:GB","region-code":"150","sub-region-code":"154"},{"name":"United States","alpha-2":"US","alpha-3":"USA","country-code":"840","iso_3166-2":"ISO 3166-2:US","region-code":"019","sub-region-code":"021"},{"name":"United States Minor Outlying Islands","alpha-2":"UM","alpha-3":"UMI","country-code":"581","iso_3166-2":"ISO 3166-2:UM"},{"name":"Uruguay","alpha-2":"UY","alpha-3":"URY","country-code":"858","iso_3166-2":"ISO 3166-2:UY","region-code":"019","sub-region-code":"005"},{"name":"Uzbekistan","alpha-2":"UZ","alpha-3":"UZB","country-code":"860","iso_3166-2":"ISO 3166-2:UZ","region-code":"142","sub-region-code":"143"},{"name":"Vanuatu","alpha-2":"VU","alpha-3":"VUT","country-code":"548","iso_3166-2":"ISO 3166-2:VU","region-code":"009","sub-region-code":"054"},{"name":"Venezuela, Bolivarian Republic of","alpha-2":"VE","alpha-3":"VEN","country-code":"862","iso_3166-2":"ISO 3166-2:VE","region-code":"019","sub-region-code":"005"},{"name":"Viet Nam","alpha-2":"VN","alpha-3":"VNM","country-code":"704","iso_3166-2":"ISO 3166-2:VN","region-code":"142","sub-region-code":"035"},{"name":"Virgin Islands, British","alpha-2":"VG","alpha-3":"VGB","country-code":"092","iso_3166-2":"ISO 3166-2:VG","region-code":"019","sub-region-code":"029"},{"name":"Virgin Islands, U.S.","alpha-2":"VI","alpha-3":"VIR","country-code":"850","iso_3166-2":"ISO 3166-2:VI","region-code":"019","sub-region-code":"029"},{"name":"Wallis and Futuna","alpha-2":"WF","alpha-3":"WLF","country-code":"876","iso_3166-2":"ISO 3166-2:WF","region-code":"009","sub-region-code":"061"},{"name":"Western Sahara","alpha-2":"EH","alpha-3":"ESH","country-code":"732","iso_3166-2":"ISO 3166-2:EH","region-code":"002","sub-region-code":"015"},{"name":"Yemen","alpha-2":"YE","alpha-3":"YEM","country-code":"887","iso_3166-2":"ISO 3166-2:YE","region-code":"142","sub-region-code":"145"},{"name":"Zambia","alpha-2":"ZM","alpha-3":"ZMB","country-code":"894","iso_3166-2":"ISO 3166-2:ZM","region-code":"002","sub-region-code":"014"},{"name":"Zimbabwe","alpha-2":"ZW","alpha-3":"ZWE","country-code":"716","iso_3166-2":"ISO 3166-2:ZW","region-code":"002","sub-region-code":"014"}];

    this.init();

}; // -- end of Application definition

