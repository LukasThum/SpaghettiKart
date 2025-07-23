if firstRun then
  print(player.id, ": ", player.name);
  state.lastSteer = 0
  state.lastAccelerate = 0
else
  -- if not player.isHuman then
  if player.name == "Mario" then

  else
    if (math.random() < 0.86) then
      local steerRange = 0.11
      local steerRand = (math.random() * 2 * steerRange) - steerRange
      state.lastSteer = clamp(-1, state.lastSteer + steerRand, 1)
      -- print("steering: ", player.id, state.lastSteer)
    else
      state.lastSteer = state.lastSteer * 0.97
    end
    steer(player.id, state.lastSteer)
    if (math.random() < 0.3) then
          accelerate(player.id)
    end
    printToScreen(4, "123456789112345678921234567")
  end
end
