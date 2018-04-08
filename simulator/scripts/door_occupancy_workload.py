
class workload:
    def __init__(self):
        self.dataset = {
            'name': 'dataset',
            'filename': '../enhants/numpy_arrays/SetupA.npy',
            'resolution_s': 30,
        }
        self.config = {
            # name and type of workload
            'name' : 'door_occupancy',
            'type' : 'reactive',
            'curve': 'curves/c098e5900064_reactive_curve.npy',
            'lambda': 500,

            'sleep_current_A' : 1.5E-6,
            'startup_energy_J': 150E-6,
            'startup_period_s': 3.4E-3,
            'event_energy_J': 8.62E-5,
            'event_period_s': 3E-3,
            'event_period_min_s': 3E-3,
            'atomic': True,
        }
