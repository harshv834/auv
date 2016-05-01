# Build Instruction
```sh
catkin_make clean
catkin_make --pkg motion_actions
catkin_make
```

# Structure
The stack contains
  - `motion_actions` contains action files.
  - `motion_forward` forward motion
  - `motion_sideward` sway motion
  - `motion_turn` rotating in horizontal plain
  - `motion_upward` vertical motion