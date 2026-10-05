local function compare(prev, a, b)
    if prev ~= 0 then
        return prev
    end
    if a < b then
        return -1
    end
    if a > b then
        return 1
    end
    return 0
end

vpicc_proto = Proto("vpicc","Virtual Smart Card Protocol")
vpicc_proto.fields.req_frame = ProtoField.framenum("vpicc.req_frame", "Request frame", base.NONE, frametype.REQUEST)
vpicc_proto.fields.resp_frame = ProtoField.framenum("vpicc.resp_frame", "Response frame", base.NONE, frametype.RESPONSE)
vpicc_proto.fields.length = ProtoField.uint16("vpicc.length", "Payload length")
vpicc_proto.fields.cmd = ProtoField.uint8("vpicc.cmd", "Reader command", base.DEC, {
    [0]="Power Off",
    [1]="Power On",
    [2]="Reset",
    [4]="Get ATR",
})
vpicc_proto.fields.atr = ProtoField.bytes("vpicc.atr", "Answer to Reset", base.SPACE)
vpicc_proto.fields.apdu_c = ProtoField.bytes("vpicc.apdu_c", "APDU request")
vpicc_proto.fields.apdu_r = ProtoField.bytes("vpicc.apdu_r", "APDU response")

local isodep_proto = Dissector.get("iso7816")
local isodep_atr_proto = Dissector.get("iso7816.atr")

function vpicc_proto.dissector(buffer,pinfo,tree)
    local direction = 0
    direction = compare(direction, pinfo.src, pinfo.dst)
    direction = compare(direction, pinfo.src_port, pinfo.dst_port)

    local conv = pinfo.conversation
    if conv[vpicc_proto] == nil then
        conv[vpicc_proto] = { requests={}, next_req=0, next_resp=0, first_req_idx={} }
    end
    local conv_data = conv[vpicc_proto]

    local function get_frame_len(buffer,pinfo,offset)
        local length = buffer:range(offset, 2):uint() + 2
        if length > 500 then
            dprint("FPM message length is too long: ", length)
            length = buffer:len()
        end
        return length
    end

    local have_frames = false
    local req_idx = conv_data.first_req_idx[pinfo.number]

    local function dissect_frame(buffer,pinfo,tree)
        if conv_data.vpcd_dir == nil then
            conv_data.vpcd_dir = direction
        end
        local is_vpcd = conv_data.vpcd_dir == direction
        if is_vpcd then
            pinfo.p2p_dir = P2P_DIR_SENT
        else
            pinfo.p2p_dir = P2P_DIR_RECV
        end

        if not have_frames then
            have_frames = true
            pinfo.cols.protocol = "VPICC"
            pinfo.cols.info:clear()
        end

        if req_idx == nil then
            if is_vpcd then
                req_idx = conv_data.next_req
            else
                req_idx = conv_data.next_resp
            end
            conv_data.first_req_idx[pinfo.number] = req_idx
        end

        local subtree = tree:add(vpicc_proto,buffer(), is_vpcd and "VPICC command" or "VPICC response")
        subtree:add(vpicc_proto.fields.length, buffer(0,2))
        local data = buffer(2,buffer:len()-2)
        local info_descr

        if is_vpcd then
            local req_data = conv_data.requests[req_idx]
            if req_data == nil then
                req_data = { req_frame = pinfo.number }
                conv_data.requests[req_idx] = req_data
                conv_data.next_req = conv_data.next_req + 1
            end
            req_idx = req_idx + 1

            if data:len() == 1 then
                subtree:add(vpicc_proto.fields.cmd, data)
                local value_labels = vpicc_proto.fields.cmd.valuestring
                info_descr = "Command: " .. (value_labels[data:uint()] or tostring(data:uint()))
                if data:uint() == 4 then
                    req_data.response_type = "atr"
                end
            else
                subtree:add(vpicc_proto.fields.apdu_c, data)
                info_descr = "APDU: " .. data:bytes():tohex()
                req_data.response_type = "apdu"
                isodep_proto(data:tvb(),pinfo,subtree)
            end

            if req_data.resp_frame ~= nil then
                subtree:add(vpicc_proto.fields.resp_frame, req_data.resp_frame)
            end
        else
            local req_data
            repeat
                req_data = conv_data.requests[req_idx]
                req_idx = req_idx + 1
                if conv_data.next_resp < req_idx then
                    conv_data.next_resp = req_idx
                end
            until req_data.response_type ~= nil
            req_data.resp_frame = pinfo.number

            if req_data.response_type == "atr" then
                subtree:add(vpicc_proto.fields.atr, data)
                info_descr = "Answer to Reset: " .. data:bytes():tohex()
                isodep_atr_proto:call(data:tvb(),pinfo,subtree)
            elseif req_data.response_type == "apdu" then
                subtree:add(vpicc_proto.fields.apdu_r, data)
                info_descr = "APDU response: " .. data:bytes():tohex()
                isodep_proto:call(data:tvb(),pinfo,subtree)
            end

            subtree:add(vpicc_proto.fields.req_frame, req_data.req_frame)
        end

        pinfo.cols.info:append("[" .. info_descr .. "]", " ")
        return buffer:len()
    end

    return dissect_tcp_pdus(buffer, tree, 2, get_frame_len, dissect_frame)
end

tcp_table = DissectorTable.get("tcp.port")
tcp_table:add(0x8C7B,vpicc_proto)
