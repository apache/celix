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

function docReady() {
    var ansi_up = new AnsiUp;

    var host = window.location.host;
    var shellSocket = new WebSocket("ws://" + host + "/shell/socket");

    shellSocket.onmessage = function (event) {
        var html = ansi_up.ansi_to_html(event.data);
        document.getElementById("console_output").innerHTML = html;
    };
    shellSocket.onopen = function (event) {
        shellSocket.send("lb");
    };

    document.getElementById("command_button").onclick = function() {
        input = document.getElementById("command_input").value;
    document.getElementById("command_input").value = "";
        shellSocket.send(input);
    };

    var input = document.getElementById("command_input");
    input.addEventListener("keyup", function(event) {
        event.preventDefault();
        if (event.key === "Enter") {
            document.getElementById("command_button").click();
        }
    });
}
