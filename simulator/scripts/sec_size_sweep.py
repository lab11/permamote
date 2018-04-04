class sweep:
    def __init__(self):
        self.sweep_vars = [[('secondary', 'capacity_J'), [i*10**exp for exp in range(-3, 3) for i in [1, 5]], 'util']]
#class sweep:
#    def __init__(self):
#        self.sweep_vars = [[('secondary', 'capacity_J'), [1], 'util']]
