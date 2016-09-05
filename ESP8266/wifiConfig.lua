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

local function close_connection(c)
    print("Close connection")
    c:close()
end

local function send_values(c)
    print("Send Values\n")
    javascript = "<script type='text/javascript'>\n" ..
                 "document.wificonfig.client_id.value = '".. client_id .. "';\n" ..
                 "document.wificonfig.server_url.value = '".. server_url .. "';\n" ..
                 "document.wificonfig.wifi_ssid.value = '".. wifi_ssid .. "';\n" ..
                 "document.wificonfig.wifi_password.value = '".. wifi_password .. "';\n" ..
                 "</script>\n"
    print(javascript)
    c:send(javascript, close_connection(c))
end

-- Send file fname via conn, asynchronously.
local function send_file(conn, fname)
    local curpos = 0
    local function cbk(c)
        file.open(fname)
        file.seek("set", curpos)
        local data = file.read(111)
        curpos = file.seek("cur", 0)
        file.close()

        if data == nil then
            print("Sending Values\n")
            close_connection(c)
        else
            c:send(data, cbk)
        end
    end
    cbk(conn)
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

wifi_ssid, wifi_password, server_url, client_id = read_wifi_credentials()

-- Setup the web server for configuation
srv=net.createServer(net.TCP, 30) --30s timeout 
srv:listen(80, function(conn) 
   local rnrn=0
   local Status = 0
   local responseBytes = 0
   local method=""
   local url=""
   local vars=""
   local sendDone=0

   conn:on("receive",function(conn, payload)
  
    print("Receive\n")
    if Status==0 then
        _, _, method, url, vars = string.find(payload, "([A-Z]+) /([^?]*)%??(.*) HTTP")
        -- print(method, url, vars)                          
    end
    
    --print("Heap   : " .. node.heap())
    --print("Payload: " .. payload)
    --print("Method : " .. method)
    --print("URL    : " .. url)
    --print("Vars   : " .. vars  .. "\n\n\n")


    -- Check if wifi-credentials have been supplied
    if vars~=nil and parse_wifi_credentials(vars) then
        url="wifiConfig.html"
    end

    if url == "favicon.ico" then
        conn:send("HTTP/1.1 404 file not found")
        responseBytes = -1
        return
    end    

    -- Only support sending one file
    url="wifiConfig.html"
    responseBytes = 0
    sendDone = 0
    
    conn:send(
                "HTTP/1.0 200 OK\r\n" ..
                "Content-Type: text/html\r\n" ..
                "Connection: close\r\n" ..
                "\r\n",
                function(cnn) send_file(cnn, "wifiConfig.html") end
            )

  end)
  
end)
print("HTTP Server: Started")


function parse_wifi_credentials(vars)
    if vars == nil or vars == "" then
        return false
    end

    local _, _, wifi_ssid = string.find(vars, "wifi_ssid\=([^&]+)")
    local _, _, wifi_password = string.find(vars, "wifi_password\=([^&]+)")
    local _, _, server_url = string.find(vars, "server_url\=([^&]+)")
    local _, _, client_id = string.find(vars, "client_id\=([^&]+)")

    if wifi_ssid == nil or wifi_ssid == "" or wifi_password == nil  or server_url == nil then
        return false
    end

    pwd_len = string.len(wifi_password)
    if pwd_len ~= 0 and (pwd_len < 8 or pwd_len > 64) then
        print("Password length should be between 8 and 64 characters")
        return false
    end

    wifi_ssid,_ = string.gsub(wifi_ssid, "%%(%x%x)", function(h) return string.char(tonumber(h, 16)) end)
    wifi_password,_ = string.gsub(wifi_password, "%%(%x%x)", function(h) return string.char(tonumber(h, 16)) end)
    server_url,_ = string.gsub(server_url, "%%(%x%x)", function(h) return string.char(tonumber(h, 16)) end)
    client_id,_ = string.gsub(client_id, "%%(%x%x)", function(h) return string.char(tonumber(h, 16)) end)

    print("New WiFi credentials received")
    print("-----------------------------")
    print("wifi_ssid     : " .. wifi_ssid)
    print("wifi_password : " .. wifi_password)
    print("server_url    : " .. server_url)
    print("client_id     : " .. client_id)

    file.open("wifi_credentials", "w+")
    file.writeline(wifi_ssid)
    file.writeline(wifi_password)
    file.writeline(server_url)
    file.writeline(client_id)
    file.flush()
    file.close()
    return true
end

