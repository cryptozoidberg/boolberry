// Ajax router
Router = function(ajaxContainer) {

    this.settings = {
        'animation': {
            'hideSpeed': 600,
            'showSpeed': 600
        },
        'defaultScreen': "index",
        'pages': [
            'routingError', 'empty', // should be the first ones

            'index', 'safes', 'market', 'contacts', 'history', 'deposits', 'settings',
            'sendG', 'buyG', 'sellG', 'buyProduct', 'sellProduct', 'safe', 'contact',

            'index0', 'safes0', 'market0', 'contacts0', 'history0', 'deposits0',
            'contact0', 'safe0', 'safe1'
        ],
        'emptyPages': [
            'screen2', 'screen3'
        ],
        'rewrite': {
            'screen4': 'safes'
        }
    };
    this.application = null;
    this.ajaxContainer = ajaxContainer;
    this.pagesToLoadCounter = 0; // how many pages are yet to load through loadPages() async. ajax
    this.appCallback = null;
    this.currentScreen = null;
    this.currentOption = null;
    this.prevScreen = null;
    var $router = this;

    // Subscribes to hash changing events and link clicks
    this.registerEvents = function() {
        $(window).hashchange(this.hashChanged);

        $(document).on('click', 'a[href*=".html"]', function() {
            var hash = $(this).attr('href').split("#")[1];
            var request = $(this).attr('href').replace('.html', '').split("#")[0];
            if (hash && hash != '') {
                location.hash = request + '/' + hash;
            } else {
                location.hash = request;
            }
            return false;
        });
    };

    // Load interface
    this.loadPages = function(callback) {
        loadingAnimationOn();

        var pages = this.settings.pages;
        this.pagesToLoadCounter = pages.length;
        for (var i in pages) {
            (function() {
                var page = pages[i];
                var url = 'ajax/' + page + '.html';
                var html = "<div class='appScreen appScreen_"+page+"'></div>";
                $router.ajaxContainer.append(html);

                $.ajax({url: url})
                    .done(function (data) {
                        $('.appScreen.appScreen_'+page).html(data)
                    })
                    .fail(function () {
                        $('.appScreen.appScreen_'+page).html(
                            $('.appScreen.appScreen_routingError').html()
                        );
                    })
                    .always(function () {
                        if (--$router.pagesToLoadCounter == 0) {
                            $router.loadingFinished();
                            callback();
                        }
                    });
            })(i);
        }
    };

    // When ajax finished its work
    this.loadingFinished = function() {
        loadingAnimationOff();
        $(window).hashchange(); // load selected screen
    };

    // Hash change event handler
    this.hashChanged = function() {
        if ($router.pagesToLoadCounter != 0) return; // pages haven't been loaded yet

        // Parse hash
        var hash = location.hash.substring(1); // remove #
        if (hash == '' || hash == '/') hash = $router.settings.defaultScreen;

        // Path separated by slash
        var option = null;
        var slashPos = hash.indexOf('/');
        if (slashPos >= 0) {
            var option = hash.substr(slashPos+1);
            hash = hash.substr(0, slashPos);
        }

        // Did it change?
        var screenChanged = false;
        if (hash != $router.currentScreen) {
            // Update routing info
            screenChanged = true;
            $router.prevScreen = $router.currentScreen;
            $router.currentScreen = hash;

            // Rewrite?
            if (hash in $router.settings.rewrite) {
                location.hash = $router.settings.rewrite[hash];
                return;
            }

            // Is it in the empty pages list?
            if ($router.settings.emptyPages.indexOf(hash) >= 0) {
                hash = 'empty';
                return;
            }

            // Is it valid at all?
            if ($router.settings.pages.indexOf(hash) < 0) {
                hash = 'routingError';
            }

            // Update sidebar buttons look
            $('.sidebar li.active').removeClass('active');
            $('.sidebar li').has('a[href="#' + hash + '"]').addClass('active');
            $('.sidebar li').has('a[href="' + hash + '.html"]').addClass('active');

            // Animate the disappearing content
            $router.hideContainer(function () {
                // And then continue

                // Switch the screen
                $('.appScreen').removeClass('active');
                $('.appScreen.appScreen_' + hash).addClass('active');

                // Do the updates
                $router.application.onScreenHide($router.prevScreen);
                $router.application.onScreenShow($router.currentScreen);

                // Scroll to the top
                window.scrollTo(0,0);

                // Do we have screenname/option?
                if (option) $router.application.showScreenOption($router.currentScreen, $router.currentOption);

                // Animation 'show'
                $router.showContainer();

                // App callback
                if (typeof $router.appCallback === 'function') $router.appCallback(hash);

                // Trigger resize event to execute some plugins relying on it
                window.dispatchEvent(new Event('resize'));
            });
        } // end of check if currentScreen is changed

        if (option != $router.currentOption || screenChanged) {
            // Update routing info
            $router.currentOption = option;

            // Scroll to the top
            window.scrollTo(0,0);

            // Do we have screenname/option?
            $router.application.showScreenOption($router.currentScreen, $router.currentOption, $router);

            // Animation 'show'
            $router.showContainer();

            // Trigger resize event to execute some plugins relying on it
            window.dispatchEvent(new Event('resize'));
        } // end of check if currentOption is changed
    };

    // Animations
    this.showContainer = function() {
        this.ajaxContainer.fadeIn( this.settings.animation.hideSpeed );
    };
    this.hideContainer = function(callback) {
        this.ajaxContainer.fadeOut( this.settings.animation.showSpeed, function() {
            callback();
        });
    };

    // Essentially, changes hash and then lets a hash change handler take care of it
    this.loadUri = function(uri) {
        location.href = '#' + uri;
    };

    // Bridge between Application and Router - used for screen changing event
    this.registerAppCallback = function(callback) {
        this.appCallback = callback;
    }

}; // -- end of Router definition