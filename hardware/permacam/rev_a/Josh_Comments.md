## SCH
- What's up with RST? Is P0.18 reset? Can you label it if so?
- I might use a real power switch for the camera...I know you have these pfets down but you need to stop leakage from so_many signals to get the pfet off...
- What's up with the cross in the lvld and fvld schematic? is it indicating a differential pair or just weird schematic design?
- Should you route CAM_Dn to the NRF? Or it just wont' work and you need to do a single wire?
- I would either remove the other  CAM_D signals or break them out to testpoints
- I'd like to hear you explain the camera data tansfer solution

## BRD
- Q5 label is unreadable
- I can't tell if  it's odd that the LTO is on the back? I assume you have thought about the mechanics of this
- Is the really tiny pad for the 2032 what you have been using? It looks small
- It looks like you have room to put the programmer on top, but again I don't know your mechanical plans
- You might run the camera clocks and data more carefully? But really since it's only running at < 10MHz it's probably fine the way it is...
- I would not route signals that close to the hole where the lens holder thing is mounted. I'm afraid the board will rip up there over time
    - If you wanted to be really careful you could route nothing on the top layer anywhere in that area but it seems overkill.

Yeah overall I think this looks good - the biggest worry is whether the data transfer to the NRF is going to work

