<h1>Arduino-WebBased-Weather-Station</h1>
<hr>
<figure>
  <img src = "https://user-images.githubusercontent.com/11696874/78469800-f15ea700-772c-11ea-9fcb-70e8d113f4eb.png">
  <figcaption> Weather Station web interface in action</figcaption>
</figure>

<h3>About this project</h3>
  <p> This project is implemented with the Arduino UNO microcontroller, the Arduino Ethernet Shield, a DS1302 Real Time Clock module and a DHT11 Temperature/Humidity sensor.</p>
  <h4> System Function</h4>
  <p> The system uses the DS1302RTC to keep the date and time which it displays on the first table of the webpage along with the temperature and humidity measurements which are made every 2 seconds. The forementioned table is shown below.</p>
  <img src= "https://user-images.githubusercontent.com/11696874/78470346-6d5aee00-7731-11ea-907f-b30ef4c0a466.png">
  <p> Every 5 minutes the system stores the measured Temperature and Humidity and uses the data to calculate the average Temperature and Humidity values per hour. The calculations are displayed on the second table along with the corresponding hour. When the table is full, the system shifts the data left so up to 24 calculations may be displayed and retrieved.
  

<h3>Required Libraries</h3>
<ul>
  <li><a href = "https://github.com/adafruit/DHT-sensor-library">Adafruit DHT sensor library</li>
  <li><a href = "https://github.com/chrisfryer78/ArduinoRTClibrary">Virtuabotix RTC library</li>
</ul>
