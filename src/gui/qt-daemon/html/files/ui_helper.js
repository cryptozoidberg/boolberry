
function inline_menu_item_select(item)
{
    if($(item).hasClass('inline_menu_bar_item_disabled'))
    {
        console.log("internal error: wrong item");
        return;
    }

    //first of all - get the last selected item
    var last_selected_item = $(item.parentNode).data("last_selected");
    if( !(!last_selected_item || last_selected_item == undefined) )
    {
        //hide prev view
        $(last_selected_item).removeClass('inline_menu_bar_selected_item');
        var view_to_hide = $(last_selected_item).attr('view_name');
        if(!(!view_to_hide || view_to_hide == undefined))
        {
            $('#'+view_to_hide).addClass('view_hidden');
        }
    }
    //lets show new view
    var view_to_show = $(item).attr('view_name');
    if(!view_to_show || view_to_show == undefined)
    {
        console.log("internal error: item dont have view_name attr");
        return;
    }
    $('#'+view_to_show).removeClass('view_hidden');
    $(item).addClass('inline_menu_bar_selected_item');
    $(item.parentNode).data("last_selected", item);
    //todo: call calback
}

function disable_tab(item, disable)
{
    if(disable)
    {
        $(item).addClass('inline_menu_bar_item_disabled');
    }else
    {
        $(item).removeClass('inline_menu_bar_item_disabled');
    }
}

function on_inline_menu_bar_item_click(e)
{
    event = e || window.event;
    var target = event.target || event.srcElement;
    inline_menu_item_select(target);
}

function init_controls()
{
    $('.inline_menu_bar_item').on('click',  on_inline_menu_bar_item_click);
    inline_menu_item_select(document.getElementById('daemon_state_view_menu'));
    //disable_tab(document.getElementById('wallet_view_menu'), true);
}

$(document).ready(function()
{
    init_controls();
});








