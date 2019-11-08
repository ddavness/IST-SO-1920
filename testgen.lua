--[[

    File: testgen.lua
    Creates tests for use within TecnicoFS - SO project, 2019/20

    Author: David Duque, nº 93698
    Usage: lua testgen.lua (requires the Lua package)

--]]

local FILE_HITRATE = 35

local CHARSET = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '_', '.',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',

    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
}

local namesInTree = {}

local function makeFName(cmd)
    if (math.random(1, 100) < FILE_HITRATE) and (#namesInTree ~= 0) then
        local i = math.random(1, #namesInTree)
        local fileExists = namesInTree[i][2]
        if (fileExists and cmd == "c") or (not fileExists and cmd == "d") then
            -- Don't create files already created!
            return makeFName(cmd)
        end

        namesInTree[i][2] = cmd == "c" and true or cmd == "d" and false or namesInTree[i][2]
        return namesInTree[i][1]
    end

    local length = math.random(1, 40)
    local name = ""

    for _ = 1, length do
        name = name .. CHARSET[math.random(1, #CHARSET)]
    end

    if name == "." or name == ".." then
        -- I don't think we can actually have files that are only dots
        return makeFName(cmd)
    else
        namesInTree[#namesInTree + 1] = {name, cmd == "c"}
        return name
    end
end

-- Main script
math.randomseed(os.time())
local n = 0

-- Make sure we have a proper number
while n <= 0 do
    print("Number of commands?")
    n = io.read("*n")
end

local cmds = {}
local cmdset = ""
print("Now assign values to each command so that commands with higher values are more common.")
print("Commands with the value zero won't appear in the test.")

for _,char in pairs({'c', 'l', 'd', 'r'}) do
    local m = -1
    while m < 0 do
        print("Probability of "..char.."?")
        m = io.read("*n")
    end
    if m > 0 then
        cmdset = cmdset..char
        for _ = 1, m do
            table.insert(cmds, char)
        end
    end
end

local fn = "inputs/luatest_"..cmdset.."_"..tostring(n).."_"..math.random(1, 10000)..".in"
print("Flushing to "..fn.."...")

local f = io.open(fn, "a")
io.output(f)

-- Header
io.write("# Testfile made by testgen.lua\n")
io.write("# Contains "..tostring(n).." commands.\n")

local lastTime = os.time()

for i = 1, n do
    local cmd = cmds[math.random(1, #cmds)]
    if cmd == 'r' then
        io.write(cmd.." "..makeFName('d').." "..makeFName('c').."\n");
    else
        io.write(cmd.." "..makeFName(cmd).."\n")
    end

    if not (os.time() - lastTime == 0) then
        lastTime = os.time()
        print(i.."/"..n.." commands written ("..(math.floor((i/n) * 1000)/10).."%)")
    end
end

print(n.."/"..n.." commands written (100.0%)")
io.close(f)
