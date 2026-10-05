local env={}; env._G=env
for _,name in ipairs({"assert","error","ipairs","pairs","next","pcall","tonumber","tostring","type","unpack","select","setmetatable","getmetatable","rawget","rawset","math","string","table"}) do env[name]=_G[name] end
local loaded={}
local function execute(path)
 assert(not path:find("..",1,true) and not path:find(":",1,true),"unsafe client path")
 local chunk=assert(loadfile(path));setfenv(chunk,env);return chunk()
end
env.dofile=execute
env.require=function(name) if not loaded[name] then loaded[name]=execute(name..".lua") or true end return loaded[name] end
execute("SystemEN/itemInfo.lua")

local rows={}
local function hex(s) return (tostring(s):gsub('.',function(c)return string.format('%02x',string.byte(c))end)) end
local function capture(kind,id,...)
 local row={kind,tostring(id)}
 for i=1,select('#',...) do row[#row+1]=hex(select(i,...)) end
 rows[#rows+1]=table.concat(row,'	')
 return true
end
env.AddItem=function(id,...)return capture('ITEM',id,...)end
env.AddItemIdentifiedDesc=function(id,text)return capture('DESC',id,text)end
env.AddItemUnidentifiedDesc=function(id,text)return capture('UNID',id,text)end
env.AddItemEffectInfo=function(id,...)return capture('EFFECT',id,...)end
env.AddItemIsCostume=function(id,...)return capture('COSTUME',id,...)end
env.AddItemPackageID=function(id,...)return capture('PACKAGE',id,...)end
assert(env.main())
table.sort(rows)
for _,row in ipairs(rows)do print(row)end
