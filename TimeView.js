// $Id$

/**
 * Copyright 2011 ILRI
 *
 * This file is part of TimeView.
 * 
 * TimeView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * TimeView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with TimeView.  If not, see <http://www.gnu.org/licenses/>.
*/

var tl;

function TimeView_onLoad(XML_source, target_id) {
  var eventSource = new Timeline.DefaultEventSource();
  var bandInfos = [
    Timeline.createBandInfo({
      width:          "75%", 
      eventSource:    eventSource,
      intervalUnit:   Timeline.DateTime.WEEK, 
      intervalPixels: 50,
      timeZone:       3
    }),
    Timeline.createBandInfo({
      overview:       true,
      width:          "25%", 
      eventSource:    eventSource,
      intervalUnit:   Timeline.DateTime.MONTH, 
      intervalPixels: 100
    })
  ];
  bandInfos[1].syncWith = 0;
  bandInfos[1].highlight = true;
  tl = Timeline.create(document.getElementById(target_id), bandInfos);
  Timeline.loadXML(XML_source, function(xml, url) { eventSource.loadXML(xml, url); });
}
