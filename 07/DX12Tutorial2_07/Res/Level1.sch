{
  "formation" : [
    {
      "name" : "A",
      "list" : [
        { "type" : "enemy0", "action" : "WaveL", "offset" : [-64, -48], "interval" : 0.05 },
        { "type" : "enemy0", "action" : "WaveL", "offset" : [0, -24], "interval" : 0 },
        { "type" : "enemy0", "action" : "WaveL", "offset" : [64, -48], "interval" : 0.05 }
      ]
    },
    {
      "name" : "B",
      "list" : [
        { "type" : "enemy1", "action" : "ZigzagL", "offset" : [0, -24], "interval" : 0 },
        { "type" : "enemy1", "action" : "ZigzagL", "offset" : [0, -24], "interval" : 0.5 },
        { "type" : "enemy1", "action" : "ZigzagL", "offset" : [0, -24], "interval" : 1 },
        { "type" : "enemy1", "action" : "ZigzagL", "offset" : [0, -24], "interval" : 1.5 },
        { "type" : "enemy1", "action" : "ZigzagL", "offset" : [0, -24], "interval" : 2 },
        { "type" : "enemy1", "action" : "ZigzagL", "offset" : [0, -24], "interval" : 2.5 }
      ]
    },
    {
      "name" : "C",
      "list" : [
        { "type" : "enemy1", "action" : "ZigzagR", "offset" : [0, -24], "interval" : 0 },
        { "type" : "enemy1", "action" : "ZigzagR", "offset" : [0, -24], "interval" : 0.5 },
        { "type" : "enemy1", "action" : "ZigzagR", "offset" : [0, -24], "interval" : 1 },
        { "type" : "enemy1", "action" : "ZigzagR", "offset" : [0, -24], "interval" : 1.5 },
        { "type" : "enemy1", "action" : "ZigzagR", "offset" : [0, -24], "interval" : 2 },
        { "type" : "enemy1", "action" : "ZigzagR", "offset" : [0, -24], "interval" : 2.5 }
      ]
    },
    {
      "name" : "D",
      "list" : [
        { "type" : "enemy0", "action" : "WaveL", "offset" : [0, -24], "interval" : 0 },
        { "type" : "enemy0", "action" : "WaveL", "offset" : [0, -24], "interval" : 0.25 },
        { "type" : "enemy0", "action" : "WaveL", "offset" : [0, -24], "interval" : 0.5 },
        { "type" : "enemy0", "action" : "WaveL", "offset" : [0, -24], "interval" : 0.75 },
        { "type" : "enemy0", "action" : "WaveR", "offset" : [0, -24], "interval" : 0 },
        { "type" : "enemy0", "action" : "WaveR", "offset" : [0, -24], "interval" : 0.25 },
        { "type" : "enemy0", "action" : "WaveR", "offset" : [0, -24], "interval" : 0.5 },
        { "type" : "enemy0", "action" : "WaveR", "offset" : [0, -24], "interval" : 0.75 }
      ]
    }
  ],
  "schedule" : [
    { "time" : 4, "event" : "A", "position" : [200, 0] },
    { "time" : 8, "event" : "A", "position" : [600, 0] },
    { "time" : 12, "event" : "A", "position" : [400, 0] },
    { "time" : 16, "event" : "B", "position" : [100, 0] },
    { "time" : 20, "event" : "C", "position" : [700, 0] },
    { "time" : 24, "event" : "D", "position" : [400, 0] },
    { "time" : 26, "event" : "A", "position" : [600, 0] },
    { "time" : 26, "event" : "A", "position" : [200, 0] }
  ]
}