
local player = getPlayer(playerId)

local checkpoint = getNextCheckpoint(playerId)
local checkpoint_v = maf.vector(
  checkpoint.locX,
  checkpoint.locY,
  checkpoint.locZ
)
local velocity_v = maf.vector(
  player.velX,
  player.velY,
  player.velZ
)

-- if player.name == "Wario" then
-- if math.random(100) < 40 then
-- end

if player.isHuman then
  if player.speed > 40 then
    -- playerInfo.cooldown = playerInfo.cooldown - 1
    -- if playerInfo.cooldown < 0 then
    --   decelerate(playerId)
    -- end
  else
    -- playerInfo.cooldown = 30
    accelerate(playerId)
  end
  -- angle = velocity_v:angle(checkpoint_v)
  -- if angle < math.pi then
  --   steer(playerId, -1)
  -- else
  --   steer(playerId, 1)
  -- end
end


setPlayerInfo(playerId, json.encode(playerInfo))
