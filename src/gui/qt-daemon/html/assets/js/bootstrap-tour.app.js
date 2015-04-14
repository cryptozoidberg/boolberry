(function() {
  $(function() {
    var $demo, duration, remaining, tour;
    $demo = $("#demo");
    duration = 5000;
    remaining = duration;
    tour = new Tour({
      onStart: function() {
        $demo.addClass("disabled", true);
        return $("#start").modal("hide"); 
      },
      onEnd: function() {
        return $demo.removeClass("disabled", true);
       
      },
      debug: true,
      steps: [
        {
         path: "#index",
          element: "#mainnav",
          placement: "bottom",
          title: "Navigation on sections application",
          content: "This menu can be minimized to icon by pressing this button",
          duration: 6000,
          reflex: true
        },
        {
          path: "#index",
          element: "#lockapp",
          placement: "top",
          title: "Lock button",
          content: "Blocks application includes screen to enter pincode for the next entry",
          duration: 5000,
          reflex: true
        },
        {
          path: "#index",
          element: "#status",
          placement: "bottom",
          title: "Indicator loading data from the network",
          content: "All users of the payment system work with a single base payments. To begin to transfer or receive payments you need to wait for a full load the bases. When you first start a continuous process, in the following only be downloaded quickly update",
          duration: 5000,
          reflex: true
        },

        {
          path: "#index",
          element: "#appactions",
          placement: "bottom",
          title: "Button bar",
          content: "Opens a form of translation coins and create proposals for the sale and purchase of foreign currency or goods",
          duration: 5000,
          backdrop: true,
          reflex: true
        },

        {
          path: "#index",
          element: "#infotablo",
          placement: "bottom",
          title: "Information board",
          content: "Will display relevant information about your data in the application",
          duration: 5000,
          backdrop: true

        },
        {
          path: "#index",
          element: "#creatsafe",
          placement: "bottom",
          title: "Create the first safe",
          content: "While there is empty, but when you sozdadete safe in this block will be displayed last added safes",
          duration: 3500,
          backdrop: true
        },
        {
          path: "#index",
          element: "#opensafe",
          placement: "bottom",
          title: "Open the safe",
          content: "If you create a file somewhere in guilders account, you can open this file here",
          duration: 3500,
          backdrop: true
        },
        {
          path: "#index",
          element: "#repairsafe",
          placement: "bottom",
          title: "Restore safe",
          content: "If you know the phrase from a digital copy (Smartsafe) account in guilders, here you can restore the safe with the help of",
          duration: 3500,
          backdrop: true
        },
        {
          path: "#index",
          element: "#widgets",
          placement: "top",
          title: "Widgets - customizable blocks of relevant information",
          content: "Каждый виджет имеет выпадающее меню с доступными типами данных. Вы сможете настроить, какую информацию выводить в каждый блок. Лишние блоки можно Rename",
          duration: 5000,
          backdrop: true
        },
        {
          path: "#index",
          element: "#addwidget",
          placement: "top",
          title: "Add widget button",
          content: "With this button you can add another widget",
          duration: 5000,
          reflex: true
        },
         {
          path: "#index",
          element: "#safelink",
          placement: "bottom",
          title: "Safes section",
          content: "Jump to section control safe",
          duration: 5000,
          reflex: true
        },
        {
          path: "#safes",
          element: "#safelist",
          placement: "top",
          title: "Safes list",
          content: "All created safes will be displayed here, you can browse them in a list or table",
          duration: 5000
        },
        {
          path: "#safes",
          element: "#samplesafe",
          placement: "bottom",
          title: "The transition to the card safe",
          content: "By clicking on the name of each safe is open information and control safe",
          duration: 5000,
          reflex: true
        },
         {
          path: "#safe",
          element: "#safestatus",
          placement: "top",
          title: "Status safe",
          content: "Visually, the status of payments, the availability of funds",
          duration: 4000,
          reflex: true
        },
         {
          path: "#safe",
          element: "#safeinfo",
          placement: "top",
          title: "Data safe",
          content: "Complete information about the safe, safe account, account aliases, binding and distribution aliases",
          duration: 3000,
          reflex: true
        },
         {
          path: "#safe",
          element: "#safehistiry",
          placement: "top",
          title: "Payment history safe",
          content: "Incoming and outgoing payments on the safe",
          duration: 3000,
          reflex: true

        },
         {
          path: "#safe",
          element: "#safemaining",
          placement: "top",
          title: "Activity in the safe production",
          content: "Schedule and History of revenues from natural",
          duration: 3000,
          reflex: true
        },
         {
          path: "#safe",
          element: "#marketlink",
          placement: "bottom",
          title: "Market",
          content: "Jump to the section for sale of goods and purchase of foreign currency ",
          duration: 5000,
          reflex: true
        },
         {
          path: "#market",
          element: "#markettabs",
          placement: "bottom",
          title: "Bookmarks bar market",
          content: "Here you can buy and sell currencies and commodities for guilders. Go to these bookmarks and see only currency or goods only",
          duration: 5000,
          reflex: true
        },
         {
          path: "#market",
          element: "#navbar-tabs",
          placement: "bottom",
          title: "Filter for offers",
          content: "If you want to fine-tune the selection of proposals, use the filter",
          duration: 5000,
          reflex: true
        },
         {
          path: "#market",
          element: "#marketfltbtns",
          placement: "bottom",
          title: "Select buttons demand - offer",
          content: "Sort respectively, only a proposal or application for purchase only",
          duration: 5000,
          reflex: true
        },
         {
          path: "#market",
          element: "#offersTbl",
          placement: "bottom",
          title: "Table offers",
          content: "Always located on top of the application fee - green, then new - yellow, crossed those removed during playback",
          duration: 5000,
          reflex: true
        },
         {
          path: "#market",
          element: "#favbtn",
          placement: "bottom",
          title: "Add to favorites",
          content: "Offers that you found interesting, you can add to favorites",
          duration: 3000,
          reflex: true
        },
        {
          path: "#market",
          element: "#moreload",
          placement: "top",
          title: "Download the following 20 offers",
          content: "Total number of applications huge, they are shown on the date of placement, exclusion - paid top",
          duration: 3000,
          reflex: true
        },
         {
          path: "#market",
          element: "#contactslink",
          placement: "bottom",
          title: "Section Contacts",
          content: "Jump to Contacts",
          duration: 5000,
          reflex: true
        },

         {
          path: "#contacts",
          title: "Contacts",
          content: "This address book to which you can add addresses of users of the system indicating the account or alias",
          duration: 5000,
          orphan: true
        },
         {
          path: "#contacts",
          element: "#historylink",
          placement: "bottom",
          title: "Section payment history",
          content: "Jump to the section payment history",
          duration: 3000,
          reflex: true
        },
        {
          path: "#history",
          title: "Payment history",
          content: "Here is the history of all payments made by the filter settings sample",
          duration: 5000,
          orphan: true,
          reflex: true
        },
         {
          path: "#history",
          element: "#maininglink",
          placement: "bottom",
          title: "Mining section",
          content: "Jump to the section Mining",
          duration: 3000,
          reflex: true
        },
         {
          path: "#deposits",
          title: "Mining section",
          content: "Production - is making a profit from the underlying funds in the account, extraction starts automatically after N days, or manually in the action menu of the safe. It also has a calculator that will show a profit forecast",
          duration: 5000,
          orphan: true,
          reflex: true
        },
        {
          path: "#index",
          title: "",
          content: "",
          duration: 30,
          orphan: true,
          reflex: true,
          onShow: function() {
           return $("#pincode").modal('hide');
          }
        },

        {
          path: "#index",
          element: "#helptoggler",
          placement: "bottom",
          title: "Help button",
          content: "Helps you navigate through the functions and buttons on each page",
          duration: 5000,
          reflex: true
        },
        {
          path: "#index",
          title: "The tour ended",
          content: "Thank you for viewing. Now you can easily start using the application. Start here, make a safe first",
          duration: 5000,
          orphan: true,
          reflex: true      
        }
      ]
    }).init();
    $(document).on("click", "[data-demo]", function(e) {
      e.preventDefault();

      if ($(this).hasClass("disabled")) {
        
        return;
      }

      tour.restart();
      return $(".tour-alert").alert("close");
    });
    $("html").smoothScroll();
    return 
  });

}).call(this);
