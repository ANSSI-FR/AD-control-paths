var OBJECT_STYLES = {
    default:                    { typeLetter:"-", fillColor:"#CCCCCC", strokeColor:"#333333" },
    user:                       { typeLetter:"u", fillColor:"#80B2FF", strokeColor:"#0047B2" },
    inetorgperson:              { typeLetter:"i", fillColor:"#80B2FF", strokeColor:"#0047B2" },
    foreignsecurityprincipal:   { typeLetter:"w", fillColor:"#FFA366", strokeColor:"#8F3900" },
    computer:                   { typeLetter:"m", fillColor:"#D65C33", strokeColor:"#661A00" },
    group:                      { typeLetter:"g", fillColor:"#70DB70", strokeColor:"#196419" },
    organizationalunit:         { typeLetter:"o", fillColor:"#CCCCCC", strokeColor:"#333333" },
    grouppolicycontainer:       { typeLetter:"x", fillColor:"#AD8533", strokeColor:"#402B00" },
    null:                       { typeLetter:"?", fillColor:"#FFFFFF", strokeColor:"#A352CC" }
};

var CENTRAL_NODE_PROPERTIES = {
    radius:         20,
    strokewidth:    "5px",
    classe:         "central",
    labelweight:    "bold",
    headmarker:     "url(#arrowheadcentral)",
    arrowX:         34,
    arrowY:         -4
};

var OTHER_NODES_PROPERTIES = {
    radius:         10,
    strokewidth:    "3px",
    classe:         "",
    labelweight:    "",
    headmarker:     "url(#arrowhead)",
    arrowX:         20,
    arrowY:         -3
};

var LABEL_DELTA = 3;
var DATA_DIR = "data/"


function getObjectStyle(d) {
  var key = d.type in OBJECT_STYLES ? d.type : "default";
  return OBJECT_STYLES[key];
}

function getObjectStyleLetter(d) {
    return getObjectStyle(d).typeLetter;
}

function getObjectStyleFillColor(d) {
    return getObjectStyle(d).fillColor;
}

function getObjectStyleStrokeColor(d) {
    return getObjectStyle(d).strokeColor;
}

function linkArc(d) {
  var dx = d.target.x - d.source.x,
      dy = d.target.y - d.source.y,
      dr = Math.sqrt(dx * dx + dy * dy);
  return "M" + d.source.x + "," + d.source.y + "A" + dr + "," + dr + " 0 0,1 " + d.target.x + "," + d.target.y;
}

function transform(d) {
  return "translate(" + d.x + "," + d.y + ")";
}

function urlParam(name) {
      name = RegExp ('[?&]' + name.replace (/([[\]])/, '\\$1') + '(?:=([^&#]*))?');
      return (window.location.href.match (name) || ['', ''])[1];
}

function loadJsonData(file) {
    data = null;
    $.ajax({
        dataType: "json",
        url: DATA_DIR + file,
        async: false,
        success: function(result){data = result},
        error: function(){document.write("failed to load json file '"+file+"'\n<br />Usage: vizu.html?json=&lt;file&gt;");}
    });
    return data;
}

function isCentralNode(d) {
    return d.dist == 0;
}

function getNodeProperties(d) {
    return isCentralNode(d) ? CENTRAL_NODE_PROPERTIES : OTHER_NODES_PROPERTIES;
}

function getMarker(d) {
    return getNodeProperties(d.target).headmarker;
}

function getStrokeWidth(d) {
    return getNodeProperties(d).strokewidth;
}

function getRadius(d) {
    return getNodeProperties(d).radius;
}

function getClass(d) {
    return getNodeProperties(d).classe;
}

function getLabelWeight(d) {
    return getNodeProperties(d).labelweight;
}

function getLabelX(d) {
    return getRadius(d) + LABEL_DELTA;
}