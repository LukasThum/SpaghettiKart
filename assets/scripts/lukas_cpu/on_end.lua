local json = json.encode(player.infos)
if (json ~= nil and json ~= "") then
  -- print(json)
  setPlayerInfos(player.id, json)
end
