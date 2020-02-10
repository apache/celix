/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

var seqnr = 0;
var sendSocketP1;

function docReady() {
    var host = window.location.host;
    var rcvSocketP1 = new WebSocket("ws://" + host + "/pubsub/default/poi1");
    var rcvSocketP2 = new WebSocket("ws://" + host + "/pubsub/default/poi2");
    sendSocketP1 = new WebSocket("ws://" + host + "/pubsub/default/poiCmd");

    rcvSocketP1.onmessage = function (event) {
        console.log(event);
        var obj = JSON.parse(event.data);
        document.getElementById("receivePoi1").innerHTML = "Received " + obj.id + " message with sequence nr: "
            + obj.seqNr + "<br/>latitude = " + JSON.stringify(obj.data.location.lat) + "<br/>longitude = " + JSON.stringify(obj.data.location.lon);
    };

    rcvSocketP2.onmessage = function (event) {
        console.log(event);
        var obj = JSON.parse(event.data);
        document.getElementById("receivePoi2").innerHTML = "Received " + obj.id + " message with sequence nr: "
            + obj.seqNr + "<br/>latitude = " + JSON.stringify(obj.data.location.lat) + "<br/>longitude = " + JSON.stringify(obj.data.location.lon);
    };
}

function submitForm() {
    var element = document.getElementById("command");
    var value = element.options[element.selectedIndex].value;
    let msg = "{\"id\":\"poiCmd\", \"major\": 1, \"minor\": 0, \"seqNr\": " + seqnr++ + ", \"data\": {" +
        "\"command\": \"" + value + "\" }}";

    console.log("Sending message: " + msg);
    sendSocketP1.send(msg);
    alert("sending message: " + msg);
    return false;
}