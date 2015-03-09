$(function () {
    var $ = window.jQuery;
    var router = new GuilderRouter($('.ajaxContainer'));
    var emulator = new GuilderEmulator();
    var backend = new GuilderBackend(emulator);
    var app = new GuilderApplication(router, backend);
});