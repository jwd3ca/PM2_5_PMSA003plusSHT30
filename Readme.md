
Code to use M5Core2 with the PM2.5 Air Quality Module along with the BTC base with SHT30 temp/humidity sensor.

I have recently completed arduino ide code to display air quality data on an M5Stack Core2 with attached
M5stack PM2.5 unit and sitting in the M5Stack BTC standing base with SHT30. This code is not exactly original, 
being baseed on various example code found around CyberSpace, but it does have some interesting features such
as influxdb/grafana posting, two independent millis timers (one for influxdb posting, one for temp/humidity display),
memory management and the use of a full-screen sprite to display the data in a flicker-free manner. 

This code works flawlessly in my own coding/living environment (as opposed to some/most of the example code I found).

Sharing in hopes that someone can use/hack it for their own project(s). 

// --------------
The colour coding for the filled circle is based on information I received from the Coway air purifier company:

Good (blue) → Moderate (green) → Unhealthy (yellow) → Very unhealthy (Red)
 
Good		Moderate	Unhealthy	Very Unhealthy
0-30(ug/m3)	31-80		81-150		>151

Data is posted to my local influxdb server and viewed via my local grafana server. See screenshots

This is my first github repository. Please don't hesitate to let me know what I've done wrong (or omitted)). Thanks.




