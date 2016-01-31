//make request for XHR weather data
var xhrRequest = function (url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);	
	};
	xhr.open(type, url);
	xhr.send();
};

//weather is requested here
function locationSuccess(pos) {
	
	//Enter own API key in order to successfully get weather from OpenWeatherMap
	//get an API key here: http://openweathermap.org/appid
	var myAPIKey = 'abc123';
	
	//construct URL
	var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
						pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey;

	//send request to OpenWeatherMap
	xhrRequest(url, 'GET',
		function(responseText) {
			//responseText contains a JSON object with weather info
			var json = JSON.parse(responseText);
			
			//TODO: allow user to determine whether they want farenheit or celsius displayed
			//temperature is in Kelvin and requires adjustment
			var temperature = Math.round(json.main.temp - 273.15); // <-- for celsius
			console.log('Temperature is ' + temperature);
			
			//conditions
			var conditions = json.weather[0].main;
			console.log('Conditions are ' + conditions);
			
			//uncomment to display entire JSON object structure
			//console.log(JSON.parse(responseText));
			
			//assemble dictionary using keys
			var dictionary = {
				'KEY_TEMPERATURE' : temperature,
				'KEY_CONDITIONS' : conditions
			};
			
			//send to pebble
			Pebble.sendAppMessage(dictionary, 
				function(e) {
					console.log('Weather info sent to pebble successfully!');
				},
				function(e) {
					console.log('Error sending weather info to Pebble!');
				}
			);
		}							
	);
}

function locationError(err) {
	console.log('Error requesting location!');
	console.log('Did you forget to enter your API key?');
}

function getWeather() {
	navigator.geolocation.getCurrentPosition(
			locationSuccess,
			locationError,
		{timeout: 15000, maximumAge: 60000}
		);
}

//listen for when the watchface is opened
Pebble.addEventListener('ready',
	function(e) {
		console.log('PebbleKit JS ready!');
		
		//get the initial weather
		getWeather();
	}
);

//listen for when AppMessage is received
Pebble.addEventListener('appmessage',
	function(e) {
		console.log('AppMessage received!');
		getWeather();
	}												
);


