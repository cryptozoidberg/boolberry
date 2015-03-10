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
        this.backend.onAppInit();
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
        var widget_number = this.getRandomInt()
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

        if (widgetName == 'activeMining') {
            doPlot("right"); // draw charts
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

    this.onScreenInit = function(screen) {
        if ($app.alreadyInitializedScreens.indexOf(screen) < 0) {
            $app.alreadyInitializedScreens.push(screen);
            if ($app.screens[screen]) {
                $app.screens[screen].init();
            }
        }

        $app.backend.updateScreen(screen);
    };

    this.getRandomInt = function(min, max) {
        return Math.floor(Math.random() * (max - min + 1) + min);
    }

    //////////////////////////////////////////////////////////////

    this.init();

}; // -- end of Application definition