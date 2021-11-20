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
    document.getElementById('power_button').addEventListener('click', powerListener);
    document.getElementById('one_cup_button').addEventListener('click', oneCupListener);
    document.getElementById('two_cups_button').addEventListener('click', twoCupsListener);
}
 
//set senseo on / off
function powerListener(event) {
    websocket.send("power");
    console.log("send msg to server: power");
}

//make one cup of coffee
function oneCupListener(event) {
    websocket.send("one_cup");
    console.log("send msg to server: one_cup");
}

//make two cups of coffee
function twoCupsListener(event) {
    websocket.send("two_cups");
    console.log("send msg to server: two_cups");
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

    //state ready
    if(event.data == "ready"){
        document.getElementById('power_button').style.backgroundColor = "green";
    }

    //state off
    else if(event.data == "off"){
        document.getElementById('power_button').style.backgroundColor = "#c2ccd6"; //color initial
    }

    //state heating or make coffee
    else if(event.data == "busy"){
        document.getElementById('power_button').style.backgroundColor = "yellow";
    }

    //state failure (no water)
    else if(event.data == "failure"){
        document.getElementById('power_button').style.backgroundColor = "red";
    }

    else if(event.data == "eng"){
        document.getElementById('one_cup_button').innerText = "ONE CUP";
        document.getElementById('two_cups_button').innerText = "TWO CUPS";
    }
}
//#endregion
