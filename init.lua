mqtt_ip = "mqttbroker"
mqtt_port = 1883
access_token = 

counter=0
lasttemp=-999
pin = 4
ow.setup(pin)

function bxor(a,b)
   local r = 0
   for i = 0, 31 do
      if ( a % 2 + b % 2 == 1 ) then
         r = r + 2^i
      end
      a = a / 2
      b = b / 2
   end
   return r
end

function getTemp()
  addr = ow.reset_search(pin)
  repeat      
      if (addr ~= nil) then
        crc = ow.crc8(string.sub(addr,1,7))
        if (crc == addr:byte(8)) then
          if ((addr:byte(1) == 0x10) or (addr:byte(1) == 0x28)) then
            ow.reset(pin)
            ow.select(pin, addr)
            ow.write(pin, 0x44, 1)
            tmr.delay(1000000)
            present = ow.reset(pin)
            ow.select(pin, addr)
            ow.write(pin,0xBE, 1)
            data = nil
            data = string.char(ow.read(pin))
            for i = 1, 8 do
              data = data .. string.char(ow.read(pin))
            end
            crc = ow.crc8(string.sub(data,1,8))
            if (crc == data:byte(9)) then
               t = (data:byte(1) + data:byte(2) * 256)
               if (t > 32768) then
                   t = (bxor(t, 0xffff)) + 1
                   t = (-1) * t
               end
            t = t * 625
            lasttemp = t
          end                   
        end
      end
    end
    addr = ow.search(pin)
  until(addr == nil)
end


function startMqtt(mq_client)
    -- init mqtt client with keepalive timer 120sec
    m = mqtt.Client("esp8266", 120, access_token, "password", 1)
    print("Connecting to MQTT broker...")
    m:connect(mqtt_ip, mqtt_port, 0, 1, 
      function(client) print("Connected to MQTT!") end,
      function(client, reason) print("Could not connect, failed reason: " .. reason) end)
    m:on("offline", function(client) print("MQTT offline") end)

    tmr.alarm(2, 300000, 1, function()
        getTemp()
        temp = lasttemp / 10000
        if temp > -100 or temp < 100 then
            m:publish("v1/devices/me/telemetry", string.format("[{\"temperature\":%d}]", math.floor(temp)), 0, 0, 
              function(client) print("Sent: "..temp) end)
        end    
    end)

end

station_cfg={}
station_cfg.ssid = 
station_cfg.pwd = 

wifi.setmode(wifi.STATION)
wifi.sta.config(station_cfg)
wifi.sta.connect()

tmr.alarm(1, 1000, 1, function()
    if wifi.sta.getip() == nil then
        print("IP unavailable, Waiting...")
    else
        tmr.stop(1)
        tmr.unregister(1)
        print("ESP8266 mode is: " .. wifi.getmode())
        print("The module MAC address is: " .. wifi.ap.getmac())
        print("Config done, IP is "..wifi.sta.getip())
        startMqtt(mqttClient)
    end
end)


