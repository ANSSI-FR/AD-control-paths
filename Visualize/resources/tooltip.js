var X_OFFSET = 30;
var Y_OFFSET = 10;

function Tooltip(tooltipId, width) {
    var tooltipId = tooltipId;

    jQuery(document).ready(function() {
        jQuery("body").append("<div class='tooltip' id='" + tooltipId + "'></div>");
        if (width) {
            jQuery("#" + tooltipId).css("width", width);
        }
        hideTooltip();
    });

    function showTooltip(content, event) {
        jQuery("#" + tooltipId).html(content);
        jQuery("#" + tooltipId).show();

        updatePosition(event);
    }

    function hideTooltip() {
        jQuery("#" + tooltipId).hide();
    }

    function updatePosition(event) {
        var ttid = "#" + tooltipId;

        var toolTipW = jQuery(ttid).width();
        var toolTipeH = jQuery(ttid).height();
        var windowY = jQuery(document).scrollTop();
        var windowX = jQuery(document).scrollLeft();
        var curX = event.pageX;
        var curY = event.pageY;
        var ttleft = ((curX) < jQuery(document).width()) ? curX - toolTipW - X_OFFSET * 2 : curX + X_OFFSET;
        if (ttleft < windowX + X_OFFSET) {
            ttleft = windowX + X_OFFSET;
        }
        if (ttleft > (jQuery(document).width() - toolTipW - (X_OFFSET * 2))) {
            ttleft = ((jQuery(document).width() - toolTipW) - (X_OFFSET * 2));
        }
        var tttop = ((curY - windowY + Y_OFFSET * 2 + toolTipeH) > jQuery(document).height()) ? curY - toolTipeH - Y_OFFSET * 2 : curY + Y_OFFSET;
        if (tttop < windowY + Y_OFFSET) {
            tttop = curY + Y_OFFSET;
        }
        if (tttop > (jQuery(document).height() - toolTipeH - (Y_OFFSET * 3))) {
            tttop = ((jQuery(document).height() - toolTipeH) - (Y_OFFSET * 3));
        }
        jQuery(ttid).css('top', tttop + 'px').css('left', ttleft + 'px');
    }

    return {
        showTooltip: showTooltip,
        hideTooltip: hideTooltip,
        updatePosition: updatePosition
    }
}

var tooltip = Tooltip("vis-tooltip");

//show tooltip 
function showDetails(d) {
    var content = '';
    if(d.name) {
      content += '<div><span class="main" >' + d.name.replace(/,/g,',<br>') + '</span></div>';
        tooltip.showTooltip(content, d3.event);
    }
    if(d.rels) {
      content += '<div><span class="main" >' + d.rels.join('<br>') + '</span></div>';
      tooltip.showTooltip(content, d3.event);
    }
};

//hide tooltip 
function hideDetails(d) {
    tooltip.hideTooltip();
};
