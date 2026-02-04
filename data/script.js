// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create Temperature Gauge
var gaugeTemp = new LinearGauge({
  renderTo: 'gauge-temperature',
  width: 120,
  height: 400,
  units: "Temperature C",
  minValue: 0,
  startAngle: 90,
  ticksAngle: 180,
  maxValue: 40,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueDec: 2,
  valueInt: 2,
  majorTicks: [
      "0",
      "5",
      "10",
      "15",
      "20",
      "25",
      "30",
      "35",
      "40"
  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 20,
          "to": 30,
          "color": "rgba(0, 128, 0, .75)"
      },
      {
          "from": 30,
          "to": 40,
          "color": "rgba(200, 50, 50, .75)"
      }
  ],
  colorPlate: "#fff",
  colorBarProgress: "#CC2936",
  colorBarProgressEnd: "#049faa",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  barWidth: 10,
}).draw();
  
// Create Humidity Gauge
var gaugeHum = new RadialGauge({
  renderTo: 'gauge-humidity',
  width: 300,
  height: 300,
  units: "Humidity (%)",
  minValue: 0,
  maxValue: 100,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueInt: 2,
  majorTicks: [
      "0",
      "20",
      "40",
      "60",
      "80",
      "100"

  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 80,
          "to": 100,
          "color": "#03C0C1"
      }
  ],
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#007F80",
  colorNeedleEnd: "#007F80",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear"
}).draw();

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var temp = myObj.temperature;
      var hum = myObj.humidity;
      gaugeTemp.value = temp;
      gaugeHum.value = hum;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

function playAlertTone() {
    const ctx = new (window.AudioContext || window.webkitAudioContext)();
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();

    osc.type = "sine";
    osc.frequency.value = 880; // A5 tone
    gain.gain.value = 0.2;

    osc.connect(gain);
    gain.connect(ctx.destination);

    osc.start();
    osc.stop(ctx.currentTime + 0.4); // 400ms beep
}

alertPlayed = false;
if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);

  source.addEventListener('sensor_data', function(e) {
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    gaugeTemp.value = myObj.temperature;
    if (myObj.temperature >= 15 && !alertPlayed) {
      // playAlertTone();
      alertPlayed = true;
    }
    if ('humidity' in myObj) {
      // Property exists - you can access myObj.humidity
      document.getElementById("humidity-card").style.display = "";
      document.querySelector('.card-grid').classList.remove('single-card');
      gaugeHum.value = myObj.humidity;
    } else {
      // Property does not exist - handle accordingly (e.g., hide the gauge or set a default)
      document.getElementById("humidity-card").style.display = "none";
      document.querySelector('.card-grid').classList.add('single-card');
    }
    
    // Update printer status if included
    if ('status' in myObj) {
      updatePrinterStatus(myObj.status);
    }
  }, false);
}

function updatePrinterStatus(status) {
  const statusElement = document.getElementById('printer-status');
  const statusText = statusElement.querySelector('.status-text');
  
  // Remove all status classes
  statusElement.classList.remove('not-ready', 'ready', 'too-hot');
  
  // Add appropriate class and text based on status
  switch(status) {
    case 'READY':
      statusElement.classList.add('ready');
      statusText.textContent = 'READY';
      break;
    case 'TOO_HOT':
    case 'TOO HOT':
      statusElement.classList.add('too-hot');
      statusText.textContent = 'TOO HOT';
      break;
    case 'NOT_READY':
    case 'NOT READY':
    default:
      statusElement.classList.add('not-ready');
      statusText.textContent = 'NOT READY';
      break;
  }
}
