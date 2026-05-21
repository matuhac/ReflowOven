#pragma once
//ulozit do PROGMEM ??
const char index_html[] = R"rawliteral(
<html>
<style>


    .centerText{
        padding: 10px 10px;
        width: 500px;
        text-align: center;
    }
    
    .divrowControl {
        background: rgba(250.0, 200.0, 0.0, 120.0);
        padding: 10px 10px;
        border: 5px solid;
        border-radius: 30px;
        width: 500px;
        text-align: center;
    }

    .divrow {
        background: rgba(0.0, 180.0, 255.0, 255.0);
        padding: 10px 10px;
        border: 5px solid;
        border-radius: 30px;
        width: 500px;
        text-align: center;
    }
    
    .divrowNastaveni {
        background: rgba(100.0, 180.0, 0.0, 255.0);
        padding: 5px 10px;
        border: 5px solid;
        border-radius: 30px;
        width: 500px;
        text-align: center;
    }
    
    .divrowDownload {
        background: rgba(250.0, 0.0, 200.0, 255.0);
        padding: 5px 10px;
        border: 5px solid;
        border-radius: 30px;
        width: 500px;
        text-align: center;
    }

    .row {
        display: flex;
        gap: 20px;
        padding: 1px 10px;
        text-align: center;
        justify-content: center;
        align-items: center;
    }
    .box{
        width: 100px;
        height: 100px;
        background: lightblue;
    }
    
    .button {
      border: none;
      color: black;
      padding: 15px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      margin: 4px 2px;
      cursor: pointer;
      transition-duration: 0.4s;
    }
    

    
    body {
    background-image: repeating-linear-gradient(
        45deg,
        silver  10px,
        white 10px,
        white 10px,
        silver 20px
    );
}  
    .button1 {border: 2px solid #04AA6D;}
    .button2 {border: 2px solid #f44336;}
    
    .button1:hover {
      background-color: #04AA6D;
      color: white;
    }
    
    .button2:hover {
      background-color: #f44336;
      color: white;
    }

    .led {
    width: 24px;
    height: 24px;
    border-radius: 50%;
    position: relative;
    display: inline-block;

    transition: all 0.3s ease;
    }

    /* Red state */
    .led.red {
    background: radial-gradient(circle at 30% 30%, #ff6666, #990000);
    box-shadow:
        0 0 5px rgba(255, 0, 0, 0.8),
        0 0 15px rgba(255, 0, 0, 0.6),
        inset -3px -3px 6px rgba(0,0,0,0.6);
    }

    /* Green state */
    .led.green {
    background: radial-gradient(circle at 30% 30%, #66ff66, #006600);
    box-shadow:
        0 0 5px rgba(0, 255, 0, 0.8),
        0 0 15px rgba(0, 255, 0, 0.6),
        inset -3px -3px 6px rgba(0,0,0,0.6);
    }

    /* Highlight */
    .led::after {
    content: "";
    position: absolute;
    top: 3px;
    left: 5px;
    width: 40%;
    height: 40%;
    border-radius: 50%;
    background: rgba(255,255,255,0.7);
    filter: blur(1px);
    }
    

</style>

<body>
    
<div class="centerText">
<div class='status-bar'>
    <div class='status-dot' id='status-dot'></div>
    <span id='status-text'>DISCONNECTED</span>
</div>

<div class="divrowControl">
<h1>Reflow oven</h1>
<h2>Ovladani Pece</h2>

<!-- ON/OFF tlacitka -->

<p>Zapnuti/Vypnuti pece:</p>

<button class="button button1" id='ON_BTN'  onclick='SendON()'>ON</button>
<button class="button button2" id='OFF_BTN' onclick='SendOFF()'>OFF</button>
</div>
</div>


<!-- Indikatory -->
<div class="divrow">
<h3>Stav Pece</h3>

    <div class="row">
        <div class="led green"></div>
        <div class='card-label' id='StavON/OFF'>Vypnuto</div>
   </div>
    
    <h4>Hodnoty ohrevu:</h4>
    
    <div class="row">
        <div class='card-label'>Stav:</div>
        <div class='card-label' id='State'></div>
    </div>
    
    <!-- HODNOTY -->
    <div class="row">
        <div class='card-label'>Setpoint:</div>
        <div class='card-label' id='Setpoint'>0.0</div>
    </div>
    
    <div class="row">
        <div class='card-label'>Teplota:    </div>
        <div class='card-label' id='TeplotaPece'>0.0</div>
    </div>
    
    <div class="row">
        <div class='card-label'>Vykon:  </div>
        <div class='card-label' id='VykonPece'>0.0</div>
    </div>
</div>

<p></p>

<div class="divrowNastaveni">
<h1>Nastaveni parametru ohrevu</h1>

<div class="row">
    <!-- KROK 1 PREHEAT -->
    <div class='card' style='margin-bottom: 16px;'>
        <div class='card-label'>Krok 1: Preheat</div>
        <div class='input-row'>
            <input
                type='number'
                id='Teplota1_ID'
                placeholder='Zadej Teplotu'
            />
            <button id='send-btn1' onclick='HandleKrok1()'>SEND</button>
        </div>
    </div>
    <!-- Soucasne nastaveni -->
    <div class='card' style='margin-bottom: 16px;'>
        <div class='card-label'>Soucasna hodnota Preheat:</div>
        <div class='card-label' id='PreheatVal'>0.0</div>
    </div>

</div>

<div class="row">
    <!-- KROK 2 SOAK -->
    <div class='card' style='margin-bottom: 16px;'>
        <div class='card-label'>Krok 2: Soak</div>
        <div class='input-row'>
            <input
                type='number'
                id='DobaTrvani2_ID'
                placeholder='Zadej Dobu trvani'
            />
            <button id='send-btn2' onclick='HandleKrok2()'>SEND</button>
        </div>
    </div>

    <!-- Soucasne nastaveni -->
    <div class='card' style='margin-bottom: 16px;'>
        <div class='card-label'>Soucasna hodnota Soak:</div>
        <div class='card-label' id='SoakVal'>0.0</div>
    </div>

</div>

<div class="row">
    <!-- KROK 3 REFLOW -->
    <div class='card' style='margin-bottom: 16px;'>
        <div class='card-label'>Krok 3: Reflow</div>
        <div class='input-row'>
            <input
                type='number'
                id='Teplota3_ID'
                placeholder='Zadej Teplotu'
            />
            <button id='send-btn3' onclick='HandleKrok3()'>SEND</button>
        </div>
    </div>

        <!-- Soucasne nastaveni -->
    <div class='card' style='margin-bottom: 16px;'>
        <div class='card-label'>Soucasna hodnota Reflow:</div>
        <div class='card-label' id='ReflowVal'>0.0</div>
    </div>
</div>
    <!-- KROK 4 COOLING zatim nenaimplementovan -->


</div>

<p></p>

<div class="divrowDownload">
<div class="centerText">
<p>Stazeni dat:</p>
<button onclick="downloadTxtFile(timeArray, dataArray, actionArray)">
    Download TXT
</button>
</div>
</div>

<script>

    // ── WebSocket Setup ──────────────────────────────────────────
    const WS_URL = 'ws://192.168.4.1/ws';
    let socket = null;
    let toggleState = false;

    let state = false;
    let prev_state = false;

    const timeArray = new Array();
    const dataArray = new Array();
    const actionArray = new Array();


    function connect() {
        socket = new WebSocket(WS_URL);

        socket.onopen = function () {
            setStatus(true);
        };

        socket.onclose = function () {
            setStatus(false);
            // Auto-reconnect after 1 seconds
            setTimeout(connect, 1000);
        };

        socket.onerror = function () {
            setStatus(false);
        };

        socket.onmessage = function (event) {
            // Parsovani prichoziho integer cisla a jeho vykresleni
            const val = parseInt(event.data);
            const display = document.getElementById('output-value');
            display.textContent = isNaN(val) ? event.data : val;
        };
    }

    // ── Status UI ───────────────────────────────────────────────
    function setStatus(connected) {
        const dot    = document.getElementById('status-dot');
        const label  = document.getElementById('status-text');

        dot.classList.toggle('connected', connected);
        label.textContent = connected ? 'Stav pripojeni: CONNECTED' : 'Stav pripojeni: DISCONNECTED';
    }

    function SendON(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        //alert('ButtonON was clicked!');
        toggleState = true;
        socket.send('HEATER:ON');

        let time_length = timeArray.length; //mely by byt stejne
        let data_length = dataArray.length;

        for(let i = 0; i < time_length; i++){
            timeArray.pop();
            dataArray.pop();
            actionArray.pop();
        }
    }

    function SendOFF(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        //alert('ButtonOFF was clicked!');
        toggleState = false;
        socket.send('HEATER:OFF');
    }

    function HandleKrok1(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        const input_teplota = document.getElementById('Teplota1_ID');

        const val_teplota   = input_teplota.value.trim();

        if(val_teplota == '' || isNaN(parseInt(val_teplota))) return;

        let stringToSend = 'TEMP:SET:0:'+parseInt(val_teplota);
        socket.send(stringToSend);
        input_teplota.value = '';
    }

        function HandleKrok2(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        const input_doba = document.getElementById('DobaTrvani2_ID');

        const val_doba      = input_doba.value.trim();

        if(val_doba == '' || isNaN(parseInt(val_doba))) return;

        let stringToSend = 'TIME:SET:1:'+parseInt(val_doba);
        socket.send(stringToSend);
        input_doba.value = '';
    }

    function HandleKrok3(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        const input_teplota = document.getElementById('Teplota3_ID');

        const val_teplota   = input_teplota.value.trim();

        if(val_teplota == '' || isNaN(parseInt(val_teplota))) return;

        let stringToSend = 'TEMP:SET:2:'+parseInt(val_teplota);
        socket.send(stringToSend);
        input_teplota.value = '';
    }

    function SendKp(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        const input_ = document.getElementById('Kp');
        const val_   = input_.value.trim();

        if(val_ == '' || isNaN(parseInt(val_))) return;

        let stringToSend = 'HEATER:CONT:P:'+parseFloat(val_);
        socket.send(stringToSend);
        input_.value = '';
    }
    function SendTi(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        const input_ = document.getElementById('Ti');
        const val_   = input_.value.trim();

        if(val_ == '' || isNaN(parseInt(val_))) return;

        let stringToSend = 'HEATER:CONT:I:'+parseFloat(val_);
        socket.send(stringToSend);
        input_.value = '';
    }
    function SendTd(){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        const input_ = document.getElementById('Td');
        const val_   = input_.value.trim();

        if(val_ == '' || isNaN(parseInt(val_))) return;

        let stringToSend = 'HEATER:CONT:D:'+parseFloat(val_);
        socket.send(stringToSend);
        input_.value = '';
    }

    function ParseIncomingMessage(msg){
        if(!socket || socket.readyState !== WebSocket.OPEN) return;

        parseMsg = msg;

        //console.log("Message received - 1: ", parseMsg);

        if(parseMsg == "HeaterState:0"){
            switchUnit2 = document.querySelector(".led");
            switchUnit2.classList.remove('red');
            switchUnit2.classList.add('green');

            textField = document.getElementById("StavON/OFF");
            textField.innerHTML = "Vypnuto";
            //console.log("Turn OFF");
            
            tempField = document.getElementById("State")
            tempField.innerHTML = ""; 

            prev_state = state;
            state = false;

        }
        else if(parseMsg == "HeaterState:1"){
            switchUnit2 = document.querySelector(".led");
            switchUnit2.classList.remove('green');
            switchUnit2.classList.add('red');

            textField = document.getElementById("StavON/OFF");
            textField.innerHTML = "Zapnuto";

            prev_state = state;
            state = true;
            //console.log("Turn ON");
            if(state != prev_state){
                let time_length = timeArray.length; //mely by byt stejne
                let data_length = dataArray.length;

                for(let i = 0; i < time_length; i++){
                timeArray.pop();
                dataArray.pop();
                actionArray.pop();
                }
            }

        }
        else{
            let arr = msg.split(":");

            let param = arr[1];

            if(arr[0] == "VYKON"){
                tempField = document.getElementById("VykonPece");
                tempField.innerHTML = param + " %";
            }
            else if(arr[0] == "TEMP"){
                tempField = document.getElementById("TeplotaPece");
                tempField.innerHTML = param + ' &#8451;';
            }
            else if(arr[0] == "SETP"){
                tempField = document.getElementById("Setpoint");
                tempField.innerHTML = param + ' &#8451;';
            }
            else if(arr[0] == "PREH"){
              tempField = document.getElementById("PreheatVal");
              tempField.innerHTML = param + ' &#8451';  
            }
            else if(arr[0] == "SOAK"){
              tempField = document.getElementById("SoakVal");
              tempField.innerHTML = param + ' s';  
            }
            else if(arr[0] == "REFL"){
              tempField = document.getElementById("ReflowVal");
              tempField.innerHTML = param + ' &#8451';  
            }
            else if(arr[0] == "STATE" && state == 1){
                if(arr[1] == "0"){
                    tempField = document.getElementById("State")
                    tempField.innerHTML = "PREHEAT";  
                }
                else if(arr[1] == "1"){
                    tempField = document.getElementById("State")
                    tempField.innerHTML = "SOAK";  
                }
                else if(arr[1] == "2"){
                    tempField = document.getElementById("State")
                    tempField.innerHTML = "REFLOW";  
                }
                else if(arr[1] == "3"){
                    tempField = document.getElementById("State")
                    tempField.innerHTML = "COOLING";  
                }
                else{
                    tempField = document.getElementById("State")
                    tempField.innerHTML = "";  
                }
            }
            else if(arr[0] == "DATA"){
                timeArray.push(arr[1]);
                dataArray.push(arr[2]);
                actionArray.push(arr[3]);
            }
        }
    }


    function buildText(timeArray, dataArray, actionArray) {
        let lines = [];

        for (let i = 0; i < timeArray.length; i++) {
            let t = timeArray[i];
            let d = dataArray[i];
            let a = actionArray[i];
            lines.push(t + "\t" + d + "\t" + a);
        }

        return lines.join("\n");
    }

    function downloadTxtFile(timeArray, dataArray, actionArray) {
        const content = buildText(timeArray, dataArray, actionArray);

        const blob = new Blob([content], { type: "text/plain" });
        const url = URL.createObjectURL(blob);

        const a = document.createElement("a");
        a.href = url;
        a.download = "data.txt";

        document.body.appendChild(a);
        a.click();

        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    // ── Start connection ─────────────────────────────────────────
    connect();
    //            onkeydown="if(event.key=='Enter') HandleSendTeplota()"

    socket.addEventListener("message", (event) => {
        //console.log("Message received: ", event.data);
        ParseIncomingMessage(event.data);
    });

</script>

</body>
</html>
)rawliteral";