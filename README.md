FEATURES TO ADD:

- [ ] Game telemetry
    - Game telemetry should totally run on another thread and send SDL events to the main thread when new data is available. This way we can avoid blocking the main thread and keep the game running smoothly. Based on the type of event, RPC updates are issued.
    - Option to eventually draw telemetry data onto image like original wtplotter.
- [ ] Discord RPC based on game telemetry
    - Show current vehicle, map, speed

- [ ] Replay parsing with WRPL library
  - Parse replays and save them into a SQLITE database. Allow displaying results into tables
  - Allow downloading server replays (if there are) and merge the data with local replay (servers do not hold chat messages so we keep those from local)
  - Allow displaying all movements onto the map image
  - Allow browsing the chat messages of a replay