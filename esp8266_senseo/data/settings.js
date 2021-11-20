//#region Startcode
window.addEventListener("load",  onload);

function  onload(event) {
    initWebSocket();      // init WebSocket
    addWebSocketEvents(); // add Eventhandler
    addHTMLEvents();      // add html Events
}
//#endregion

//#region Events
function addHTMLEvents() {
    document.getElementById('language_button').addEventListener('click', langListener);
    document.getElementById('save_button').addEventListener('click', wifiListener);
}
 
//send language settings to server
function langListener(event) {
    var lang;

    if (document.getElementById("language_button").innerText == "set english") {
      lang = "eng";
      document.getElementById("language_button").innerText = "set german";
    } else {
      lang = "ger";
      document.getElementById("language_button").innerText = "set english";
    }

    var data = { lang: lang };

    websocket.send(JSON.stringify(data));
    console.log("send msg to server: language settings " + JSON.stringify(data));
}

//send wifi settings to server
function wifiListener(event) {
    var ssid = document.getElementById("ssid").value;
    var password = document.getElementById("password").value;
    var data = { ssid: ssid, password: password };
    
    websocket.send(JSON.stringify(data));
    console.log("send msg to server: wifi settings" + JSON.stringify(data));
}
//#endregion

//# WebSocket Events 
function addWebSocketEvents() {
    websocket.onopen    = onOpen;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    //get state
    websocket.send("INFO");

    console.log("send msg to server: INFO");
}
  
function onMessage(event) {
    
    console.log("get new msg from server: " + event.data);

    if(event.data == "eng"){
        document.getElementById('language_button').innerText = "set german";
    }
}
//#endregion
