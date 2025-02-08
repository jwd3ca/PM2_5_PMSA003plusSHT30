Code to use M5Core2 with the PM2.5 Air Quality Module along with the BTC base with SHT30 temp/humidity sensor.

Currently using local git for revision control. No more .GOLD files :)

The colour coding for the filled circle is based on information I received from the Coway air purifier company:

Good (blue) → Moderate (green) → Unhealthy (yellow) → Very unhealthy (Red)
 
Good		Moderate	Unhealthy	Very Unhealthy
0-30(ug/m3)	31-80		81-150		>151

Data is posted to my local influxdb server and viewed via my local grafana server. See screenshots




