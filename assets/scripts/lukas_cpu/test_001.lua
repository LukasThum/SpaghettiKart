if firstRun then
  print(player.id, ": ", player.name);
  state.lastSteer = 0
  state.lastAccelerate = 0
else
  -- if player.isHuman then
    local steerRange = 0.1
    local steerRand = (math.random() * 2 * steerRange) - steerRange
    state.lastSteer = clamp(-1, state.lastSteer + steerRand, 1)
    print("steering: ", player.id, state.lastSteer)
    -- cpu steers differently
    steer(player.id, state.lastSteer)
    if (math.random() > 0.1) then
      accelerate(player.id)
    end
  -- end
end
