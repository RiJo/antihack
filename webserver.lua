--------------------------------------------------------------------------------
-- antihack-1.6.0
--------------------------------------------------------------------------------
-- Note: all pipes (stderr, stdin and stdout) are cut when program is daemonized
--------------------------------------------------------------------------------

-- called when an incoming connection is established
function connection_established(timestamp, ip, port)
    -- not in use, connection_closed() used instead to get duration
end

-- called when an existing connection is closed
function connection_closed(timestamp, ip, port, duration)
    ---- log connection ----
    local file = io.open("connections.log", "a")
    file:write(timestamp .. "\t" .. ip .. ":" .. port .. "\t" .. duration .. "s\n")
    file:close() 
end

-- if the connection should be closed (rejected) or forked
function reject_connection(timestamp, ip, port)
    return false
end

-- message which is sent back to client after the connection is established,
-- nil == no reply
function reply_message(timestamp, ip, port)
    return nil
end

-- if the server should wait for more data or close the connection
function wait_for_more_data(timestamp, ip, port, last_incoming_data)
    return (string.len(last_incoming_data) == 0)
end

-- executed everytime data is received and sends back the return value to client
-- nil == no reply
function data_received(timestamp, ip, port, data)
    ---- log data ----
    local file = io.open(ip .. ":" .. port .. ".log", "a")
    file:write(data)
    file:close()
    ---- send reply ----
    return "<hml><body>foo bar</body></html>"
end