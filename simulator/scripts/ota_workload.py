
class workload:
    def __init__(self):
        self.config = {
            # name and type of workload
            'name' : 'sense_and_send',
            'type' : 'periodic',
            'period_s': 7*24*60*60, # update every week

            'sleep_current_A' : 1.5E-6,
            'startup_energy_J': 1.02E-3,
            'startup_period_s': 376E-3,
            'event_energy_J': 5.3E-3*3.3*50E3*8/75E3,
            'atomic': False,
            'event_period_s': 50E3*8/75E3,
            'event_period_min_s': 0.5,
        }
