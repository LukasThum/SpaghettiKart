if player.isHuman then
  if player.speed > 60 then
    decelerate(player.id)
    -- player.infos.cooldown = 3
    -- if player.infos.cooldown > 2  then
    -- else
      -- player.infos.cooldown = 1
      -- player.infos.cooldown += player.infos.cooldown - 1
    -- end
  else
    player.infos.cooldown = 30
  end
    accelerate(player.id)
  if math.random(100) < 20 then
    -- print("current speed : ", player.speed)
    -- print("cooldown: ", player.infos.cooldown)
  end
  print(player.infos.cooldown)
end
