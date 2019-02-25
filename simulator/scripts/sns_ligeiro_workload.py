
class workload:
    def __init__(self):
        self.dataset = {
            'name': 'dataset',
            'filename': 'exp_data/light_irradiance-Permamote-ble-c098e5110032_clean.npy',
            'resolution_s': 60,
        }
        self.config = {
            # name and type of workload
            'name' : 'sense_and_send',
            'type' : 'periodic',
            #'period_s': 1,

            'sleep_current_A' : 1E3,#1.5E-6,
            'startup_energy_J': 0,#150E-6,
            'startup_period_s': 0,#3.4E-3,
            'event_energy_J': .5*500E-6*(2.2**2 - 0.8**2),#5E-4 + 8.62E-5,
            'atomic': True,
            'event_period_s': 50E-3,#510E-3,
            'event_period_min_s': 50E-3,#510E-3,
        }
