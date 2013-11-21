
var aq = {
	"api_key" : "47E454ED-86CD-436B-86F2-7431AF8C4BC2",
	"configureUrl" : "http://spangborn.github.io/pebble-air-quality/index.html",
	"distance" : localStorage.getItem("distance"),
	"endpoint" : "http://www.airnowapi.org/aq/observation/latLong/current/?format=application/json",
	"locationOptions" : {
		"timeout": 15000,
		"maximumAge": 60000
	},
	"maxAppMessageRetries" : 3,
	"appMessageTimeout" : 3000
};

// From https://github.com/Neal/pebble-vlc-remote/blob/master/src/js/pebble-js-app.js
aq.sendAppMessage = function(message, numTries, transactionId) {
        numTries = numTries || 0;
        if (numTries < aq.maxAppMessageRetries) {
                numTries++;
                console.log('Sending AppMessage to Pebble: ' + JSON.stringify(message));
                Pebble.sendAppMessage(
                        message, function() {}, function(e) {
                                console.log('Failed sending AppMessage for transactionId:' + e.data.transactionId + '. Error: ' + e.data.error.message);
                                setTimeout(function() {
                                        aq.sendAppMessage(message, numTries, e.data.transactionId);
                                }, aq.appMessageTimeout);
                        }
                );
        } else {
                console.log('Failed sending AppMessage for transactionId:' + transactionId + '. Not trying again. Message: ' + JSON.stringify(message));
        }
}

// If no localStorage found, use a default
if (!aq.distance) aq.distance = "20";

/*
 	AQI Values  Levels of Health Concern  		Colors
	0 to 50 	Good  							Green
	51 to 100 	Moderate  						Yellow
	101 to 150  Unhealthy for Sensitive Groups  Orange
	151 to 200  Unhealthy 						Red
	201 to 300  Very Unhealthy  				Purple
	301 to 500  Hazardous 						Maroon
 */

aq.getData = function (lat,lon) {
	var url = aq.endpoint + "&distance=" + aq.distance + "&API_KEY=" + aq.api_key + "&latitude=" + lat + "&longitude=" + lon;
	var aqi = 101;

	console.log("URL: " + url);
	var req = new XMLHttpRequest();
	req.open('GET', url, true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				console.log(req.responseText);
				response = JSON.parse(req.responseText);
				var temperature, icon, city;
				if (response && response.length > 1) {
					
					var msg = {};
					var aqiResult = response[0];
					msg.p_i = parseInt(aqiResult.Category.Number) - 1;
					msg.p_a = " " + aqiResult.AQI;
					msg.p_l = aqiResult.Category.Name;
					msg.c = aqiResult.ReportingArea;
					Pebble.sendAppMessage(msg);	

					console.log(JSON.stringify(msg));
				    
				    msg = {};
		    		aqiResult = response[1];
		    		msg.o_i = parseInt(aqiResult.Category.Number) - 1;
					msg.o_a = " " + aqiResult.AQI;
					msg.o_l = aqiResult.Category.Name;
					
				    aq.sendAppMessage(msg);

					console.log(JSON.stringify(msg));

		  		} else {
		    		aq.sendAppMessage({
		    			"c" : "",
		    			"p_a" : "",
		    			"p_l" : "No Data",
		    			"p_i" : 5,
		    			"o_a": "",
		    			"o_l" : "",
		    			"o_i": 5
		    		});
		  		}
			}
		}
	};
	req.send(null);
};

aq.locationSuccess = function (pos) {
  //console.log(JSON.stringify(pos));
  var coordinates = pos.coords;
  aq.getData(coordinates.latitude, coordinates.longitude);
}

aq.locationError = function (err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  aq.sendAppMessage({
    "c":"Loc Unavailable"
  });
}

Pebble.addEventListener("ready", function(e) {
	//console.log("connect!" + e.ready);
	locationWatcher = window.navigator.geolocation.getCurrentPosition(aq.locationSuccess, aq.locationError, aq.locationOptions);
	//console.log(e.type);
});		

Pebble.addEventListener("appmessage", function(e) {
	window.navigator.geolocation.getCurrentPosition(aq.locationSuccess, aq.locationError, aq.locationOptions);
	//console.log(e.type);
	//console.log(e.payload.aci);
	//console.log("message!");
});

Pebble.addEventListener("showConfiguration", function(e) {
	Pebble.openURL(aq.configureUrl);
});
Pebble.addEventListener("webviewclosed",
  function(e) {

    if (e.response) {

    	var configuration = JSON.parse(e.response);
    	console.log("Configuration window returned: " + JSON.stringify(configuration));
    	
    	console.log("Distance: " + configuration.distance);
    	localStorage.setItem("distance", configuration.distance);  

		aq.distance = configuration.distance;
		window.navigator.geolocation.getCurrentPosition(aq.locationSuccess, aq.locationError, aq.locationOptions);
	}
  }
);
