// Randomly shuffles strings. Mostly the emulator needs it
String.prototype.shuffle = function () {
    var a = this.split(""),
        n = a.length;

    for(var i = n - 1; i > 0; i--) {
        var j = Math.floor(Math.random() * (i + 1));
        var tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
    return a.join("");
}

// helper for pseudo-associative arrays in JS - work like .length but does it on objects
Object.size = function(obj) {
    var size = 0, key;
    for (key in obj) {
        if (obj.hasOwnProperty(key)) size++;
    }
    return size;
};

// Main stuff
$(function () {
    var $ = window.jQuery;
    var router   = new Router($('.ajaxContainer'));
    var emulator = new Emulator();
    var backend  = new Backend(emulator);
    var app      = new Application(router, backend);
});