#pragma once

#include <util/string/type.h>

namespace NKikimr::NVisual {

const std::string_view FG_SVG_HEADER = R"scr(<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" width="%.2f" height="%.2f" onload="init(evt)" viewBox="0 0 %.2f %.2f" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<defs><linearGradient id="background" y1="0" y2="1" x1="0" x2="0"><stop stop-color="#eeeeee" offset="5%%"/><stop stop-color="#eeeeb0" offset="95%%"/>
</linearGradient></defs><style type="text/css">.graphElement:hover { stroke:black; stroke-width:0.5; cursor:pointer; }</style>" )scr";

const std::string_view FG_SVG_FOOTER = R"scr(</svg>)scr";

const std::string_view FG_SVG_BACKGROUND = R"scr(<rect x="0" y="0" width="%.2f" height="%.2f" fill="url(#background)" />)scr";

const std::string_view FG_SVG_TITLE = R"scr(<text text-anchor="middle" x="600.00"
        y="%.2f" font-size="17" font-family="Verdana" fill="rgb(0, 0, 0)">%s</text>)scr";

const std::string_view FG_SVG_INFO_BAR = R"scr(
<text id="infoBar" text-anchor="left" x="10.00" y="%.2f" font-size="12" font-family="Verdana" fill="rgb(0, 0, 0)"> </text>)scr";

const std::string_view FG_SVG_RESET_ZOOM = R"scr(
<text
        id="resetZoom" onclick="resetZoom()" style="opacity:0.0;cursor:pointer" text-anchor="left" x="10.00" y="24.00"
        font-size="12" font-family="Verdana" fill="rgb(0, 0, 0)">Reset Zoom</text>)scr";

const std::string_view FG_SVG_SEARCH =R"scr(
        <text id="search" onmouseover="onSearchHover()" onmouseout="onSearchOut()" onclick="startSearch()" style="opacity:0.1;cursor:pointer"
        text-anchor="left" x="%.2f" y="24.00" font-size="12" font-family="Verdana"
        fill="rgb(0, 0, 0)">Search</text><text id="matched" text-anchor="left" x="1090.00" y="1637.00" font-size="12"
        font-family="Verdana" fill="rgb(0, 0, 0)"> </text> )scr";

const std::string_view FG_SVG_GRAPH_ELEMENT = R"scr(
<g class="graphElement" onmouseover="onGraphMouseOver(this)" onmouseout="onGraphMouseOut()" onclick="zoom(this)">
    <title>%s</title><rect data-type="%s" x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="%s" />
    <text text-anchor="left" x="%.2f" y="%.2f" font-size="12" font-family="Verdana" fill="rgb(0, 0, 0)">%s</text>
</g>)scr";

const std::string_view FG_SVG_SCRIPT = R"scr(<script type="text/ecmascript"><![CDATA[var nametype = 'Function:';
var fontSize = 12;
var fontWidth = 0.59;
var xpad = 10;
]]><![CDATA[var infoBar, searchButton, foundText, svg;
function init(evt) {
    infoBar = document.getElementById("infoBar").firstChild;
    searchButton = document.getElementById("search");
    foundText = document.getElementById("matched");
    svg = document.getElementsByTagName("svg")[0];
    searching = 0;
}
// Show element title in bottom bar
function onGraphMouseOver(node) {		// show
    info = getNodeTitle(node);
    infoBar.nodeValue = nametype + " " + info;
}
function onGraphMouseOut() {			// clear
    infoBar.nodeValue = ' ';
}
// ctrl-F for search
window.addEventListener("keydown",function (e) {
        if (e.keyCode === 114 || (e.ctrlKey && e.keyCode === 70)) {
            e.preventDefault();
            startSearch();
        }
})
// functions
function findElement(parent, name, attr) {
    var children = parent.childNodes;
    for (var i=0; i<children.length;i++) {
        if (children[i].tagName == name)
            return (attr != undefined) ? children[i].attributes[attr].value : children[i];
    }
    return;
}
function backupAttribute(element, attr, value) {
    if (element.attributes[attr + ".bk"] != undefined) return;
    if (element.attributes[attr] == undefined) return;
    if (value == undefined) value = element.attributes[attr].value;
    element.setAttribute(attr + ".bk", value);
}
function restoreAttribute(element, attr) {
    if (element.attributes[attr + ".bk"] == undefined) return;
    element.attributes[attr].value = element.attributes[attr + ".bk"].value;
    element.removeAttribute(attr + ".bk");
}
function getNodeTitle(element) {
    var text = findElement(element, "title").firstChild.nodeValue;
    return (text)
}

function adjustText(element) {
    var textElement = findElement(element, "text");
    var rect = findElement(element, "rect");
    var width = parseFloat(rect.attributes["width"].value) - 3;
    var title = findElement(element, "title").textContent.replace(/\\([^(]*\\)\$/,"");
    textElement.attributes["x"].value = parseFloat(rect.attributes["x"].value) + 3;
    // Not enought space for any text
    if (2*fontSize*fontWidth > width) {
        textElement.textContent = "";
        return;
    }
    textElement.textContent = title;
    // Enough space for full text
    if (/^ *\$/.test(title) || textElement.getSubStringLength(0, title.length) < width) {
        return;
    }

    for (var x=title.length-2; x>0; x--) {
        if (textElement.getSubStringLength(0, x+2) <= width) {
            textElement.textContent = title.substring(0,x) + "..";
            return;
        }
    }
    textElement.textContent = "";
}


function zoomChild(element, x, ratio) {
    if (element.attributes != undefined) {
        if (element.attributes["x"] != undefined) {
            backupAttribute(element, "x");
            element.attributes["x"].value = (parseFloat(element.attributes["x"].value) - x - xpad) * ratio + xpad;
            if(element.tagName == "text") element.attributes["x"].value = findElement(element.parentNode, "rect", "x") + 3;
        }
        if (element.attributes["width"] != undefined) {
            backupAttribute(element, "width");
            element.attributes["width"].value = parseFloat(element.attributes["width"].value) * ratio;
        }
    }
    if (element.childNodes == undefined) return;
    for(var i=0, child=element.childNodes; i<child.length; i++) {
        zoomChild(child[i], x - xpad, ratio);
    }
}
function zoomParent(element) {
    if (element.attributes) {
        if (element.attributes["x"] != undefined) {
            backupAttribute(element, "x");
            element.attributes["x"].value = xpad;
        }
        if (element.attributes["width"] != undefined) {
            backupAttribute(element, "width");
            element.attributes["width"].value = parseInt(svg.width.baseVal.value) - (xpad*2);
        }
    }
    if (element.childNodes == undefined) return;
    for(var i=0, child=element.childNodes; i<child.length; i++) {
        zoomParent(child[i]);
    }
}

function zoomElement(element, type, xmin, xmax, ymin, ratio) {
    var rect = findElement(element, "rect").attributes;

    if(rect["data-type"].value != type) {
        return;
    }

    var currentX = parseFloat(rect["x"].value);
    var currentWidtn = parseFloat(rect["width"].value);
    var comparisionOffset = 0.0001;

    if (parseFloat(rect["y"].value) > ymin) {
        if (currentX <= xmin && (currentX+currentWidtn+comparisionOffset) >= xmax) {
            element.style["opacity"] = "0.5";
            zoomParent(element);
            element.onclick = function(element){resetZoom(); zoom(this);};
            adjustText(element);
        }
        else {
            element.style["display"] = "none";
        }
    }
    else {
        if (currentX < xmin || currentX + comparisionOffset >= xmax) {
            element.style["display"] = "none";
        }
        else {
            zoomChild(element, xmin, ratio);
            element.onclick = function(element){zoom(this);};
            adjustText(element);
        }
    }

}
function zoom(node) {
    var attr = findElement(node, "rect").attributes;
    var type = attr["data-type"].value
    var width = parseFloat(attr["width"].value);
    var xmin = parseFloat(attr["x"].value);
    var xmax = parseFloat(xmin + width);
    var ymin = parseFloat(attr["y"].value);
    var ratio = (svg.width.baseVal.value - 2*xpad) / width;
    var resetZoomBtn = document.getElementById("resetZoom");
    resetZoomBtn.style["opacity"] = "1.0";
    var el = document.getElementsByTagName("g");
    for(var i=0;i<el.length;i++){
        zoomElement(el[i], type, xmin, xmax, ymin, ratio)
    }
}

function resetElementZoom(element) {
    if (element.attributes != undefined) {
        restoreAttribute(element, "x");
        restoreAttribute(element, "width");
    }

    if (element.childNodes == undefined) return;
    for(var i=0, children=element.childNodes; i<children.length; i++) {
        resetElementZoom(children[i]);
    }
}

function resetZoom() {
    var resetZoomBtn = document.getElementById("resetZoom");
    resetZoomBtn.style["opacity"] = "0.0";
    var element = document.getElementsByTagName("g");

    for(i=0;i<element.length;i++) {
        element[i].style["display"] = "block";
        element[i].style["opacity"] = "1";
        resetElementZoom(element[i]);
        adjustText(element[i]);
    }
}
// search
function dropSearch() {
    var el = document.getElementsByTagName("rect");
    for (var i=0; i < el.length; i++) {
        restoreAttribute(el[i], "fill")
    }
}
function startSearch() {
    if (!searching) {
        var pattern = prompt("Enter a string to search (regexp allowed)", "");
        if (pattern != null) {
            search(pattern)
        }
    } else {
        dropSearch();
        searching = 0;
        searchButton.style["opacity"] = "0.1";
        searchButton.firstChild.nodeValue = "Search"
        foundText.style["opacity"] = "0.0";
        foundText.firstChild.nodeValue = ""
    }
}

function search(pattern) {
    var regex = new RegExp(pattern);
    var graphElements = document.getElementsByTagName("g");
    for (var i = 0; i < graphElements.length; i++) {
        var element = graphElements[i];
        if (element.attributes["class"].value != "graphElement")
            continue;
        var titleFunc = (getNodeTitle(element));
        var rect = findElement(element, "rect");

        if (titleFunc == null || rect == null)
            continue;

        if (titleFunc.match(regex)) {
            backupAttribute(rect, "fill");
            rect.attributes["fill"].value = 'rgb(120,80,230)';
            searching = 1;
        }
    }
    if (searching)
    {
        searchButton.style["opacity"] = "1.0";
        searchButton.firstChild.nodeValue = "Reset Search"
    }
}
function onSearchHover() {
    searchButton.style["opacity"] = "1.0";
}
function onSearchOut() {
    if (searching) {
        searchButton.style["opacity"] = "1.0";
    } else {
        searchButton.style["opacity"] = "0.1";
    }
}
]]></script> )scr";
}
