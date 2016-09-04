function read_wifi_credentials()
    local wifi_ssid
    local wifi_password

    if file.open("wifi_credentials", "r") then
        wifi_ssid = file.read("\n")
        wifi_ssid = string.format("%s", wifi_ssid:match( "^%s*(.-)%s*$" ))
        wifi_password = file.read("\n")
        wifi_password = string.format("%s", wifi_password:match( "^%s*(.-)%s*$" ))
        file.close()
    end

    if wifi_ssid ~= nil and wifi_ssid ~= "" and wifi_password ~= nil then
        return wifi_ssid, wifi_password
    end
    return nil, nil
end


-- Wi-Fi credentials.
local cfg = {
    ssid = "SepticMon",
    pwd = "SepticMon"
}

-- Print some useful information.
print("setting up Wi-Fi...")
print("SSID: \"" .. cfg.ssid .. "\" password: \"" .. cfg.pwd .. "\"")
wifi.setmode(wifi.SOFTAP)
wifi.ap.config(cfg)
print(wifi.ap.getip())

-- Setup the web server for configuation
srv=net.createServer(net.TCP) 
srv:listen(80, function(conn) 
   local rnrn=0
   local Status = 0
   local responseBytes = 0
   local method=""
   local url=""
   local vars=""

   conn:on("receive",function(conn, payload)
  
    if Status==0 then
        _, _, method, url, vars = string.find(payload, "([A-Z]+) /([^?]*)%??(.*) HTTP")
        -- print(method, url, vars)                          
    end
    
--    print("Heap   : " .. node.heap())
--    print("Payload: " .. payload)
--     print("Method : " .. method)
--      print("URL    : " .. url)
--      print("Vars   : " .. vars  .. "\n\n\n")


    -- Check if wifi-credentials have been supplied
    if vars~=nil and parse_wifi_credentials(vars) then
        url="wifiConfig.html"
    end

    if url == "favicon.ico" then
        conn:send("HTTP/1.1 404 file not found")
        responseBytes = -1
        return
    end    

    -- Only support one sending one file
    url="wifiConfig.html"
    responseBytes = 0
    
    conn:send("HTTP/1.1 200 OK\r\n\r\n")
    
  end)
  
  conn:on("sent",function(conn) 
    if responseBytes>=0 and method=="GET" then
        if file.open(url, "r") then            
            file.seek("set", responseBytes)
            local line=file.read(512)
            file.close()
            if line then
                conn:send(line)
                responseBytes = responseBytes + 512    

                if (string.len(line)==512) then
                    return
                end
            end
        end        
    end

    conn:close() 
  end)
end)
print("HTTP Server: Started")


function parse_wifi_credentials(vars)
    if vars == nil or vars == "" then
        return false
    end

    local _, _, wifi_ssid = string.find(vars, "wifi_ssid\=([^&]+)")
    local _, _, wifi_password = string.find(vars, "wifi_password\=([^&]+)")

    if wifi_ssid == nil or wifi_ssid == "" or wifi_password == nil then
        return false
    end

    pwd_len = string.len(wifi_password)
    if pwd_len ~= 0 and (pwd_len < 8 or pwd_len > 64) then
        print("Password length should be between 8 and 64 characters")
        return false
    end

    print("New WiFi credentials received")
    print("-----------------------------")
    print("wifi_ssid     : " .. wifi_ssid)
    print("wifi_password : " .. wifi_password)

    file.open("wifi_credentials", "w+")
    file.writeline(wifi_ssid)
    file.writeline(wifi_password)
    file.flush()
    file.close()
    return true
end

