--------------------------------------------------------------------------------
-- antihack-1.7.0
--------------------------------------------------------------------------------
-- Note: all pipes (stderr, stdin and stdout) are cut when program is daemonized
--------------------------------------------------------------------------------

-- called when an incoming connection is established
function connection_established(timestamp, ip, port)
    -- unused
end

-- called when an existing connection is closed
function connection_closed(timestamp, ip, port, duration)
    -- unused
end

-- if the connection should be closed (rejected) or forked
function reject_connection(timestamp, ip, port)
    return false
end

-- message which is sent back to client after the connection is established,
-- nil == no reply
function welcome_message(timestamp, ip, port)
    return nil
end

-- if the server should wait for more data or close the connection
function wait_for_more_data(timestamp, ip, port, last_incoming_data)
    return (string.len(last_incoming_data) == 0)
end

-- executed everytime data is received and sends back the return value to client
-- nil == no reply
function data_received(timestamp, ip, port, data)
    page, matches = string.gsub(data, ".*GET /(.*) HTTP.*", "%1")
    if matches == 1 then
        if (string.len(page) == 0) then
            page = "index.html" -- default page
        end
        local file = io.open(page, "r")
        if file then
            code = file:read()
            file:close()
            return code
        else
            return "404" -- page not found
        end
    else
        return "400" -- bad request
    end
end