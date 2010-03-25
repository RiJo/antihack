
-- called for each incoming connection
function incoming_connection(timestamp, ip, port)
    io.write("incoming connection (time " .. timestamp .. "): " .. ip .. " : " .. port .. "\n")
end

-- if reply message should be sent back to client before closing connection
function send_reply()
    return true
end

-- message which is sent back to client before closing connection
function reply_message()
    return "ya simple hacker, I'll get revenge!\n"
end