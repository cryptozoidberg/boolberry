$(function () {
    var $ = window.jQuery;
    var router   = new Router($('.ajaxContainer'));
    var emulator = new Emulator();
    var backend  = new Backend(emulator);
    var app      = new Application(router, backend);
});