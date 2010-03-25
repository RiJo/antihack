--------------------------------------------------------------------------------
-- antihack-1.3.0
--------------------------------------------------------------------------------
-- Notes:
--   * reply_message() is only executed when send_reply() returns true
--   * data_received() is only executed if close_connection() returns false
--   * all pipes (stderr, stdin and stdout) are cut when program is daemonized
--------------------------------------------------------------------------------

-- called when a connection is established
function connection_established(timestamp, ip)
    local file = io.open("rejected.log", "a")
    file:write(timestamp .. ";" .. ip .. "\n")
    file:close() 
end

-- called when a connection is closed
function connection_closed(timestamp, ip)
    -- ignored
end

-- if reply message should be sent back to client after connecting
function send_reply()
    return true
end

-- if the connection should be closed or just forked and left in eternity
function close_connection()
    return false
end

-- message which is sent back to client immediately after connecting
function reply_message(timestamp, ip)
    return "ya simple hacker, i'll get revenge!\n"
end

-- executed everytime data is received
function data_received(timestamp, ip, data)
    -- ignored
end