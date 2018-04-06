
class workload:
    def __init__(self):
        self.dataset = {
            'name': 'dataset',
            'filename': '../enhants/numpy_arrays/SetupA.npy',
            'resolution_s': 30,
        }
        self.config = {
            # name and type of workload
            'name' : 'ota_update',
            'type' : 'random',
            'period_s': 7*24*60*60, # update every 2 week

            'sleep_current_A' : 1.5E-6,
            'startup_energy_J': 150E-6,
            'startup_period_s': 3.4E-3,
            'event_energy_J': 5.3E-3*3.3*50E3*8/75E3,
            'atomic': False,
            'event_period_s': 50E3*8/75E3,
            'event_period_min_s': 0.5,
        }
