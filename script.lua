-- Notes:
--   * reply_message() is only executed when send_reply() returns true
--   * pipes (stderr, stdin and stdout) are cut when program is daemonized

-- called for each incoming connection
function incoming_connection(timestamp, ip, port)
    local file = io.open("rejected.log", "a")
    file:write(timestamp .. ";" .. ip .. ";" .. port .. "\n")
    file:close() 
end

-- if reply message should be sent back to client before closing connection
function send_reply()
    return true
end

-- if the connection should be closed or just forked and left in eternity
function close_connection()
    return false
end

-- message which is sent back to client before closing connection
function reply_message()
    return "ya simple hacker, i'll get revenge!\n"
end