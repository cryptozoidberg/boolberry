// Ajax router
Router = function(ajaxContainer) {

    this.settings = {
        'animation': {
            'hideSpeed': 600,
            'showSpeed': 600
        },
        'defaultUri': "index",
        'pages': [
            'routingError', 'empty', // should be the first ones

            'index', 'safes', 'market', 'contacts', 'history', 'deposits', 'settings',
            'sendG', 'buyG', 'sellG', 'buyProduct', 'sellProduct', 'safe',

            'index0', 'safes0', 'market0', 'contacts0', 'history0', 'deposits0', 'settings0',
            'sendG0', 'buyG0', 'sellG0', 'buyProduct0', 'sellProduct0', 'contact0', 'safe0', 'safe1',
            'safe2'
        ],
        'emptyPages': [
            'screen2', 'screen3'
        ],
        'rewrite': {
            'screen4': 'safes'
        }
    };
    this.ajaxContainer = ajaxContainer;
    this.lastHash = null;
    this.pagesToLoadCounter = 0; // how many pages are yet to load through loadPages() async. ajax
    this.appCallback = null;
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
        var hash = location.hash.substring(1);
        if (hash == '' || hash == '/') hash = $router.settings.defaultUri;

        // Did it change?
        if (hash != $router.lastHash) {
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

                // Scroll to the top
                window.scrollTo(0,0);

                // Animation 'show'
                $router.showContainer();

                // App callback
                if (typeof $router.appCallback === 'function') $router.appCallback(hash);

                // Trigger resize event to execute some plugins relying on it
                window.dispatchEvent(new Event('resize'));
            });
        }
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