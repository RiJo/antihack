--------------------------------------------------------------------------------
-- antihack-1.5.0
--------------------------------------------------------------------------------
-- Notes:
--   * reply_message() is only executed when send_reply() returns true
--   * data_received() is only executed if close_connection() returns false
--   * all pipes (stderr, stdin and stdout) are cut when program is daemonized
--------------------------------------------------------------------------------

-- called when a connection is established
function connection_established(timestamp, ip, port)
    -- not in use, connection_closed() used instead to get duration
end

-- called when a connection is closed
function connection_closed(timestamp, ip, port, duration)
    ---- log connection ----
    local file = io.open("connections.log", "a")
    file:write(timestamp .. "\t" .. ip .. ":" .. port .. "\t" .. duration .. "s\n")
    file:close() 
end

-- if reply message should be sent back to client after connecting
function send_reply(timestamp, ip, port)
    return true
end

-- if the connection should be closed or just forked and left in eternity
function close_connection(timestamp, ip, port)
    return not (port < 10000)
end

-- message which is sent back to client immediately after connecting
function reply_message(timestamp, ip, port)
    return "ya simple hacker, i'll get revenge!\n"
end

-- executed everytime data is received and sends back the return-value to client
function data_received(timestamp, ip, port, data)
    ---- log data ----
    local file = io.open(ip .. ":" .. port .. ".log", "a")
    file:write(data)
    file:close()
    ---- send reply ----
    return nil
end