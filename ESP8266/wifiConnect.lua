
function read_wifi_credentials()
    local wifi_ssid
    local wifi_password

    if file.open("wifi_credentials", "r") then
        wifi_ssid = file.read("\n")
        wifi_ssid = string.format("%s", wifi_ssid:match( "^%s*(.-)%s*$" ))
        wifi_password = file.read("\n")
        wifi_password = string.format("%s", wifi_password:match( "^%s*(.-)%s*$" ))
        server_url = file.read("\n")
        server_url = string.format("%s", server_url:match( "^%s*(.-)%s*$" ))
        client_id = file.read("\n")
        client_id = string.format("%s", client_id:match( "^%s*(.-)%s*$" ))
        file.close()
    end

    if wifi_ssid ~= nil and wifi_ssid ~= "" and wifi_password ~= nil then
        return wifi_ssid, wifi_password, server_url, client_id
    end
    return "","","",""
end

function try_connecting(wifi_ssid, wifi_password)
    wifi.setmode(wifi.STATION)
    wifi.sta.config(wifi_ssid, wifi_password)
    wifi.sta.connect()

    tmr.alarm(0, 500, 1, function()
        if wifi.sta.getip()==nil then
          print("Connecting to AP...")
        else
          tmr.stop(1)
          tmr.stop(0)
          print("Connected as: " .. wifi.sta.getip())
        end
    end)

    tmr.alarm(1, 50000, 0, function()
        if wifi.sta.getip()==nil then
            tmr.stop(0)
            print("Failed to connect to \"" .. wifi_ssid .. "\"")
        end
    end)
end

function payload(data)
  return "POST /septicMon/updateData.php HTTP/1.1\r\n"
    .. "Host: localhost\r\n"
    .. "User-Agent: foo/7.43.0\r\n"
    .. "Accept: */*\r\n"
    .. "Content-Type: application/json\r\n"
    .. "Content-Length: " .. string.len(data) .. "\r\n\r\n"
    .. data
end

function connect_to_client(data)

   wifi_ssid, wifi_password,server_url,client_id = read_wifi_credentials()
   
   if wifi_ssid ~= nil and wifi_password ~= nil and wifi.sta.getip()==nil then
       print("Connecting to WIFI with")
       print("Retrieved stored WiFi credentials")
       print("---------------------------------")
       print("wifi_ssid     : " .. wifi_ssid)
       print("wifi_password : " .. wifi_password)
       print("server_url    : " .. server_url)
       print("client_id     : " .. client_id)
       print("")
       try_connecting(wifi_ssid, wifi_password)
   end
   
   print("Wifi Connected:")
   print(wifi.sta.getip())
   
   sk=net.createConnection(net.TCP, 0)
   sk:on("connection", function()
       sk:send(payload(data))
         end)
   sk:on("receive", function(sck, c) print(c) end )
   print("Sending: " .. data)
   sk:connect(80,"192.168.1.10")
   
end

function send_data(data)
    tmr.alarm(1, 50000, 0, connect_to_client(data))
end
