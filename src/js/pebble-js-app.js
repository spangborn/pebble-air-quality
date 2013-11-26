
var aq = {
	"api_key" : "47E454ED-86CD-436B-86F2-7431AF8C4BC2",
	"configureUrl" : "http://spangborn.github.io/pebble-air-quality/index.html",
	"distance" : localStorage.getItem("distance"),
	"endpoint" : "http://www.airnowapi.org/aq/observation/latLong/current/?format=application/json",
	"locationOptions" : {
		"timeout": 15000,
		"maximumAge": 60000
	},
	"maxAppMessageRetries" : 6,
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
	aq.sendAppMessage({"c": "Loading..."});

	console.log("URL: " + url);
	var req = new XMLHttpRequest();
	req.open('GET', url, true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				console.log(req.responseText);
				response = JSON.parse(req.responseText);
				var temperature, icon, city;
<<<<<<< HEAD
				if (response && response.length > 0) {
					for (i=0;i<response.length;i++) {
						if (response[i].ParameterName == "PM2.5") {
							var aqiResult = response[i];
							var msg = {
				        		"i"	: aq.getIconFromAQI(aqiResult.AQI),
								"a"	: "PM2.5 " + aqiResult.AQI,
								"l" : aq.getLevelFromAQI(aqiResult.AQI),
								"c"	: aqiResult.ReportingArea
				        	};
				        	console.log(JSON.stringify(msg));
							Pebble.sendAppMessage(msg);
				    	}					
					}
		  		} else {
		    		Pebble.sendAppMessage({
		    			"c" : "Available",
		    			"a" : "",
		    			"l" : "No Data",
		    			"i" : 5
		    		});
=======
				if (response && response.length > 1) {
					var data = 0;
					for (var i = 0; i< response.length; i++) {
						var aqiResult = response[i];
						if (aqiResult.ParameterName == "PM2.5") {
							var msg = {};
							
							msg.p_i = parseInt(aqiResult.Category.Number) - 1;
							msg.p_a = " " + aqiResult.AQI;
							msg.p_l = aqiResult.Category.Name;
							msg.c = aqiResult.ReportingArea;
							aq.sendAppMessage(msg);							
						}
						else if (aqiResult.ParameterName == "O3") {
							var msg = {};
							
							msg.o_i = parseInt(aqiResult.Category.Number) - 1;
							msg.o_a = " " + aqiResult.AQI;
							msg.o_l = aqiResult.Category.Name;
							msg.c = aqiResult.ReportingArea;
						
							aq.sendAppMessage(msg);
						}	
					}
>>>>>>> origin/o3-and-pm25
		  		}
			}
		}
	};
	req.send(null);
};

aq.locationSuccess = function (pos) {
  var coordinates = pos.coords;
  aq.getData(coordinates.latitude, coordinates.longitude);
}

aq.locationError = function (err) {
  console.warn('location error (' + err.code + '): ' + err.message);
<<<<<<< HEAD
  Pebble.sendAppMessage({
    "c":"Loc Unavailable",
    "a":"N/A"
=======
  aq.sendAppMessage({
    "c":"Location Unavailable"
>>>>>>> origin/o3-and-pm25
  });
}

Pebble.addEventListener("ready", function(e) {
	locationWatcher = window.navigator.geolocation.getCurrentPosition(aq.locationSuccess, aq.locationError, aq.locationOptions);
});		

Pebble.addEventListener("appmessage", function(e) {
	window.navigator.geolocation.getCurrentPosition(aq.locationSuccess, aq.locationError, aq.locationOptions);
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

