local maf = require 'maf'
local json = require "json"

local function clamp(min, value, max)
  return math.max(min, math.min(max, value))
end

-- add some calculated fields to the player object
function getPlayer(playerId)
  local player = getPlayerRaw(playerId)
  player.loc =  maf.vector(
    player.locX,
    player.locY,
    player.locZ
  )
  -- check if current player is queried player
  player.isCurrent = (player.id == playerId)
  return player
end

local player = getPlayer(playerId)
local state = json.decode(getPlayerState(playerId))
