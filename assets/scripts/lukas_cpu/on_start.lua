local maf = require 'maf'
local json = require "json"

local function clamp(min, value, max)
  return math.max(min, math.min(max, value))
end

-- query player with playerId
function getPlayer(playerId)
  local player = getPlayerRaw(playerId)
  player.isCurrent = (player.id == playerId)
  player.loc =  maf.vector(
    player.locX,
    player.locY,
    player.locZ
  )
  return player
end

-- load next checkpoint for playerId
function getNextCheckpoint(playerId)
  local checkpoint = getCheckpointRaw(playerId)
  checkpoint.loc =  maf.vector(
    checkpoint.locX,
    checkpoint.locY,
    checkpoint.locZ
  )
  return checkpoint
end

function printToScreen(row, text)
  setDebugText(row, text)
end

local player = getPlayer(playerId)
local state = json.decode(getPlayerState(playerId))
