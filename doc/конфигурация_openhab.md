# Пример конфигурации для  Openhab

```
MQTT_ADDRESS
MQTT_USER
MQTT_PASSWORD
```

## Things
#### mqtt.things
Bridge mqtt:broker:local [ 
  host=MQTT_ADDRESS, 
  secure=false, 
  username=MQTT_USER, 
  password=MQTT_PASSWORD
] {

Thing ...

```
Thing topic fan_esp32 "Вентилятор esp32" [ availabilityTopic="C0:49:EF:6B:D8:4C/status", payloadAvailable="Online", payloadNotAvailable="Offline" ] {

      Type switch : state "Состояние" [
        stateTopic="C0:49:EF:6B:D8:4C/status", on="Online", off="Offline"
      ]

      Type switch : fan "Вентилятор" [
        stateTopic="C0:49:EF:6B:D8:4C/switch/state" , on="ON", off="OFF",
        commandTopic="C0:49:EF:6B:D8:4C/switch/c", on="1", off="0"
      ]

      Type number : humidity "Влажность" [
        stateTopic="C0:49:EF:6B:D8:4C/sensor/humidity"
      ]
      
      Type number : lohum "Low Hummidity Range" [
        stateTopic="C0:49:EF:6B:D8:4C/sensor/lowHum",
        commandTopic="C0:49:EF:6B:D8:4C/sensor/c/lowHum",
        min=0,
        max=100,
        unit="%"
      ]

      Type number : hihum "High Humidity Range" [
        stateTopic="C0:49:EF:6B:D8:4C/sensor/highHum",
        commandTopic="C0:49:EF:6B:D8:4C/sensor/c/highHum",
        min=0,
        max=100,
        unit="%"
      ]


      Type number : temperature  "Температура" [
        stateTopic="C0:49:EF:6B:D8:4C/sensor/temperature"
      ]

      Type number : lotemp  "Low Temperature Range" [
        stateTopic="C0:49:EF:6B:D8:4C/sensor/lowTemp",
        commandTopic="C0:49:EF:6B:D8:4C/sensor/c/lowTemp",
        unit="°C",
        min=-50,
        max=50
      ]

      Type number : hitemp  "High Temperature Range" [
        stateTopic="C0:49:EF:6B:D8:4C/sensor/highTemp",
        commandTopic="C0:49:EF:6B:D8:4C/sensor/c/highTemp",
        unit="°C",
        min=-50,
        max=50
      ] 

      Type number : delay  "Fan delay" [
        stateTopic="C0:49:EF:6B:D8:4C/sensor/fanDelay",
        commandTopic="C0:49:EF:6B:D8:4C/sensor/c/fanDelay",
        unit="sec",
        min=0
      ]
  }
```
  
  }

  ## Items

  ### fans.items

```
  Group   gFan32 "Вентилятор ESP-32"    <fan> (loc_Cabinet,g_Sensors) ["Fan"]

Switch fan_esp32_state "Статус подключения" <f7:antenna_radiowaves_left_right> 	(gFan32) ["Status"] {
	channel="mqtt:topic:local:fan_esp32:state", on="Online", off="Offline"
}

Switch fan_esp32_fan "Вытяжка" <oh:switch>	(gFan32,loc_Cabinet) ["Fan","Switch"]  {
	channel="mqtt:topic:local:fan_esp32:fan"
}


Number:Dimensionless fan_esp32_humidity "Влажность  [%d]" <humidity> (gFan32,loc_Cabinet,gHumidities)      
	["Measurement","Humidity"] 
		{ channel="mqtt:topic:local:fan_esp32:humidity" }

Number:Temperature fan_esp32_temperature "Температура [%.2f °C]" <temperature> (gFan32,loc_Cabinet,gTemperatures) ["Measurement","Temperature"] {
        channel="mqtt:topic:local:fan_esp32:temperature"
}

Number  fan_esp32_lohum "Low Humidity range [%.2f %]" <humidity> (gFan32,loc_Cabinet)      ["Fan","Setpoint"] {
        channel="mqtt:topic:local:fan_esp32:lohum"

}

Number  fan_esp32_hihum "High Humidity range [%.2f %]" <humidity> (gFan32,loc_Cabinet)      ["Fan","Setpoint"] {
        channel="mqtt:topic:local:fan_esp32:hihum"

}

Number  fan_esp32_lotemp "Low Temperature range [%.2f °C]" <temperature_cold> (gFan32,loc_Cabinet)      ["Fan","Setpoint"] {
        channel="mqtt:topic:local:fan_esp32:lotemp"

}

Number  fan_esp32_hitemp "High Temperature range [%.2f °C]" <temperature_hot> (gFan32,loc_Cabinet)      ["Fan","Setpoint"] {
        channel="mqtt:topic:local:fan_esp32:hitemp"

}

Number 	fan_esp32_delay "Fan delay [%d sec]" <clock> (gFan32,loc_Cabinet)      ["Fan","Setpoint"] {
        channel="mqtt:topic:local:fan_esp32:delay"

}
```