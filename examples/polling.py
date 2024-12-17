import remotecaptury as rc

# you can also add the port if it's different than 2101
rc.connect("127.0.0.1")

# sync remote and local clocks
rc.startSynchronizationLoop()

# stream compressed (0x100) poses (0x001)
rc.startStreaming(0x101)





rc.stopStreaming()

rc.disconnect()
