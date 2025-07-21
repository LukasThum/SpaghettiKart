local maf = require 'maf'
local json = require "json"

function getPlayer(playerId)
  local player = getPlayerRaw(playerId)
  player.loc =  maf.vector(
    player.locX,
    player.locY,
    player.locZ
  )
  player.isCurrent = (player.id == playerId)
  infos = getPlayerInfos(player.id)
  -- print(infos)
  player.infos = json.decode("{}")
  return player
end

local player = getPlayer(playerId)
