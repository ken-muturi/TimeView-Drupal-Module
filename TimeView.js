// $Id$

var tl;

function TimeView_onLoad(XML_source, target_id) {
  var eventSource = new Timeline.DefaultEventSource();
  var bandInfos = [
    Timeline.createBandInfo({
      width:          "75%", 
      eventSource:    eventSource,
      intervalUnit:   Timeline.DateTime.WEEK, 
      intervalPixels: 50,
      timezone:       3
    }),
    Timeline.createBandInfo({
      overview:       true,
      width:          "25%", 
      eventSource:    eventSource,
      intervalUnit:   Timeline.DateTime.MONTH, 
      intervalPixels: 100,
      timezone:       3
    })
  ];
  bandInfos[1].syncWith = 0;
  bandInfos[1].highlight = true;
  tl = Timeline.create(document.getElementById(target_id), bandInfos);
  Timeline.loadXML(XML_source, function(xml, url) { eventSource.loadXML(xml, url); });
}
