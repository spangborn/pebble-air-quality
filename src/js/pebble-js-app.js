
var aq = {
	"api_key" : "47E454ED-86CD-436B-86F2-7431AF8C4BC2",
	"configureUrl" : "http://spangborn.github.io/pebble-air-quality/index.html",
	"distance" : localStorage.getItem("distance"),
	"endpoint" : "http://www.airnowapi.org/aq/observation/latLong/current/?format=application/json",
	"locationOptions" : {
		"timeout": 15000,
		"maximumAge": 60000
	}
};

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

aq.getIconFromAQI = function (aqi) {
  if (aqi <= 50) {
    return 0;
  } else if (aqi <= 100) {
    return 1;
  } else if (aqi <= 150) {
    return 2;
  } else if (aqi <= 200) {
  	return 3;
  } else if (aqi <= 300) {
  	return 4;
  } else {
  	return 5;
  }
};

aq.getLevelFromAQI = function (aqi) {
  if (aqi <= 50) {
    return "Good";
  } else if (aqi <= 100) {
    return "Moderate";
  } else if (aqi <= 150) {
    return "Unhealthy for Sensitive";
  } else if (aqi <= 200) {
  	return "Unhealthy";
  } else if (aqi <= 300) {
  	return "Very Unhealthy";
  } else {
  	return "Hazardous";
  }
};

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
				if (response && response.length > 0) {
					for (i=0;i<response.length;i++) {
						if (response[i].ParameterName == "PM2.5") {
							var aqiResult = response[i];
							var msg = {
				        		"icon"	: aq.getIconFromAQI(aqiResult.AQI),
								"aqi"	: "PM2.5 " + aqiResult.AQI,
								"aqiLevel" : aq.getLevelFromAQI(aqiResult.AQI),
								"city"	: aqiResult.ReportingArea
				        	};
				        	console.log(JSON.stringify(msg));
							Pebble.sendAppMessage(msg, function(){}, function(e) {
								console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message);
							});
				    	}					
					}
		  		} else {
		    		Pebble.sendAppMessage({
		    			"city" : "Available",
		    			"aqi" : "",
		    			"aqiLevel" : "No Data",
		    			"icon" : 5
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
  Pebble.sendAppMessage({
    "city":"Loc Unavailable",
    "aqi":"N/A"
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

